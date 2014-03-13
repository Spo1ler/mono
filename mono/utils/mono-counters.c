/*
 * Copyright 2006-2010 Novell
 * Copyright 2011 Xamarin Inc
 */

#include <stdlib.h>
#include <glib.h>
#include "mono-counters-internals.h"

typedef struct _MonoCounter MonoCounter;

struct _MonoCounter {
	MonoCounter *next;
	const char *name;
	void *addr;
	MonoCounterType type;
	MonoCounterCategory category;
	MonoCounterUnit unit;
	MonoCounterVariance variance;
};

static MonoCounter *counters = NULL;
static int valid_mask = 0;

/**
 * mono_counters_enable:
 * @section_mask: a mask listing the sections that will be displayed
 *
 * This is used to track which counters will be displayed.
 */
void
mono_counters_enable (int section_mask)
{
	valid_mask = section_mask & MONO_COUNTER_SECTION_MASK;
}

void
mono_counters_register_full (MonoCounterCategory category, const char *name, MonoCounterType type, MonoCounterUnit unit, MonoCounterVariance variance, void *addr)
{
	MonoCounter *counter;
	if (!(type & valid_mask))
		return;
	counter = malloc (sizeof (MonoCounter));
	if (!counter)
		return;
	counter->name = name;
	counter->addr = addr;
	counter->type = type;
	counter->category = category;
	counter->unit = unit;
	counter->variance = variance;
	counter->next = NULL;

	/* Append */
	if (counters) {
		MonoCounter *item = counters;
		while (item->next)
			item = item->next;
		item->next = counter;
	} else {
		counters = counter;
	}

}

static void*
mono_counters_alloc_space (int size)
{
	//FIXME actually alloc memory from perf-counters
	return g_malloc (size);
}

void*
mono_counters_new (MonoCounterCategory category, const char *name, MonoCounterType type, MonoCounterUnit unit, MonoCounterVariance variance)
{
	const int sizes[] = { 4, 8, sizeof (void*) };
	void *addr;

	g_assert (type >= MONO_COUNTER_TYPE_INT && type <= MONO_COUNTER_TYPE_WORD);

	addr = mono_counters_alloc_space (sizes [type]);
	mono_counters_register_full (category, name, type, unit, variance, addr);

	return addr;
}

static int
section_to_category (int type)
{
	switch (type & MONO_COUNTER_SECTION_MASK) {
	case MONO_COUNTER_JIT:
		return MONO_COUNTER_CAT_JIT;
	case MONO_COUNTER_GC:
		return MONO_COUNTER_CAT_GC;
	case MONO_COUNTER_METADATA:
		return MONO_COUNTER_CAT_METADATA;
	case MONO_COUNTER_GENERICS:
		return MONO_COUNTER_CAT_GENERICS;
	case MONO_COUNTER_SECURITY:
		return MONO_COUNTER_CAT_SECURITY;
	default:
		g_error ("Invalid section %x", type & MONO_COUNTER_SECTION_MASK);
	}
}
/**
 * mono_counters_register:
 * @name: The name for this counters.
 * @type: One of the possible MONO_COUNTER types, or MONO_COUNTER_CALLBACK for a function pointer.
 * @addr: The address to register.
 *
 * Register addr as the address of a counter of type type.
 * Note that @name must be a valid string at all times until
 * mono_counters_dump () is called.
 *
 * It may be a function pointer if MONO_COUNTER_CALLBACK is specified:
 * the function should return the value and take no arguments.
 */
void 
mono_counters_register (const char* name, int type, void *addr)
{
	MonoCounterCategory cat = section_to_category (type);
	MonoCounterType counter_type;
	MonoCounterUnit unit = MONO_COUNTER_UNIT_NONE;


	switch (type & MONO_COUNTER_TYPE_MASK) {
	case MONO_COUNTER_INT:
	case MONO_COUNTER_UINT:
		counter_type = MONO_COUNTER_TYPE_INT;
		break;
	case MONO_COUNTER_LONG:
	case MONO_COUNTER_ULONG:
	case MONO_COUNTER_DOUBLE:
		counter_type = MONO_COUNTER_TYPE_LONG;
		break;
	case MONO_COUNTER_WORD:
		counter_type = MONO_COUNTER_TYPE_WORD;
		break;
	case MONO_COUNTER_STRING:
		g_error ("IMPLEMENT ME");
	case MONO_COUNTER_TIME_INTERVAL:
		counter_type = MONO_COUNTER_TYPE_LONG;
		unit = MONO_COUNTER_UNIT_TIME;
		break;
	default:
		g_error ("Invalid type %x", type & MONO_COUNTER_TYPE_MASK);
	}

	mono_counters_register_full (cat, name, counter_type, unit, MONO_COUNTER_UNIT_VARIABLE, addr);
}


typedef int (*IntFunc) (void);
typedef guint (*UIntFunc) (void);
typedef gint64 (*LongFunc) (void);
typedef guint64 (*ULongFunc) (void);
typedef gssize (*PtrFunc) (void);
typedef double (*DoubleFunc) (void);
typedef char* (*StrFunc) (void);

#define ENTRY_FMT "%-36s: "
static void
dump_counter (MonoCounter *counter, FILE *outfile) {
	switch (counter->type) {
#if SIZEOF_VOID_P == 4
	case MONO_COUNTER_TYPE_WORD:
#endif
	case MONO_COUNTER_TYPE_INT:
		fprintf (outfile, ENTRY_FMT "%d\n", counter->name, *(int*)counter->addr);
		break;

#if SIZEOF_VOID_P == 8
	case MONO_COUNTER_TYPE_WORD:
#endif	
	case MONO_COUNTER_TYPE_LONG:{
		gint64 value = *(gint64*)counter->addr;
		if (counter->unit == MONO_COUNTER_UNIT_TIME)
			fprintf (outfile, ENTRY_FMT "%.2f ms\n", counter->name, (double)value / 1000.0);
		else
			fprintf (outfile, ENTRY_FMT "%lld\n", counter->name, value);
		break;
	}
	}
}

static const char
section_names [][10] = {
	"JIT",
	"GC",
	"Metadata",
	"Generics",
	"Security"
};

static void
mono_counters_dump_category (MonoCounterCategory category, FILE *outfile)
{
	MonoCounter *counter = counters;
	while (counter) {
		if (counter->category == category)
			dump_counter (counter, outfile);
		counter = counter->next;
	}
}

/**
 * mono_counters_dump:
 * @section_mask: The sections to dump counters for
 * @outfile: a FILE to dump the results to
 *
 * Displays the counts of all the enabled counters registered. 
 */
void
mono_counters_dump (int section_mask, FILE *outfile)
{
	int i, j;
	section_mask &= valid_mask;
	if (!counters)
		return;
	for (j = 0, i = MONO_COUNTER_JIT; i < MONO_COUNTER_LAST_SECTION; j++, i <<= 1) {
		if ((section_mask & i)) {
			fprintf (outfile, "\n%s statistics\n", section_names [j]);
			mono_counters_dump_category (section_to_category (i), outfile);
		}
	}

	fflush (outfile);
}

/**
 * mono_counters_cleanup:
 *
 * Perform any needed cleanup at process exit.
 */
void
mono_counters_cleanup (void)
{
	MonoCounter *counter = counters;
	counters = NULL;
	while (counter) {
		MonoCounter *tmp = counter;
		counter = counter->next;
		free (tmp);
	}
}

static MonoResourceCallback limit_reached = NULL;
static uintptr_t resource_limits [MONO_RESOURCE_COUNT * 2];

/**
 * mono_runtime_resource_check_limit:
 * @resource_type: one of the #MonoResourceType enum values
 * @value: the current value of the resource usage
 *
 * Check if a runtime resource limit has been reached. This function
 * is intended to be used by the runtime only.
 */
void
mono_runtime_resource_check_limit (int resource_type, uintptr_t value)
{
	if (!limit_reached)
		return;
	/* check the hard limit first */
	if (value > resource_limits [resource_type * 2 + 1]) {
		limit_reached (resource_type, value, 0);
		return;
	}
	if (value > resource_limits [resource_type * 2])
		limit_reached (resource_type, value, 1);
}

/**
 * mono_runtime_resource_limit:
 * @resource_type: one of the #MonoResourceType enum values
 * @soft_limit: the soft limit value
 * @hard_limit: the hard limit value
 *
 * This function sets the soft and hard limit for runtime resources. When the limit
 * is reached, a user-specified callback is called. The callback runs in a restricted
 * environment, in which the world coult be stopped, so it can't take locks, perform
 * allocations etc. The callback may be called multiple times once a limit has been reached
 * if action is not taken to decrease the resource use.
 *
 * Returns: 0 on error or a positive integer otherwise.
 */
int
mono_runtime_resource_limit (int resource_type, uintptr_t soft_limit, uintptr_t hard_limit)
{
	if (resource_type >= MONO_RESOURCE_COUNT || resource_type < 0)
		return 0;
	if (soft_limit > hard_limit)
		return 0;
	resource_limits [resource_type * 2] = soft_limit;
	resource_limits [resource_type * 2 + 1] = hard_limit;
	return 1;
}

/**
 * mono_runtime_resource_set_callback:
 * @callback: a function pointer
 * 
 * Set the callback to be invoked when a resource limit is reached.
 * The callback will receive the resource type, the resource amount in resource-specific
 * units and a flag indicating whether the soft or hard limit was reached.
 */
void
mono_runtime_resource_set_callback (MonoResourceCallback callback)
{
	limit_reached = callback;
}


