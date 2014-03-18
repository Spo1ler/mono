#ifndef __MONO_COUNTERS_INTERNALS_H__
#define __MONO_COUNTERS_INTERNALS_H__

#include "mono-counters.h"
#include "mono-compiler.h"

typedef enum {
	MONO_COUNTER_CAT_JIT,
	MONO_COUNTER_CAT_GC,
	MONO_COUNTER_CAT_METADATA,
	MONO_COUNTER_CAT_GENERICS,
	MONO_COUNTER_CAT_SECURITY,

	MONO_COUNTER_CAT_THREAD,
	MONO_COUNTER_CAT_THREADPOOL,
	MONO_COUNTER_CAT_SYS,

	MONO_COUNTER_CAT_MAX
} MonoCounterCategory;

typedef enum {
	MONO_COUNTER_TYPE_INT, /* 4 bytes */
	MONO_COUNTER_TYPE_LONG, /* 8 bytes */
	MONO_COUNTER_TYPE_WORD, /* machine word */
	MONO_COUNTER_TYPE_DOUBLE,

	MONO_COUNTER_TYPE_MAX
} MonoCounterType;

typedef enum {
	MONO_COUNTER_UNIT_NONE,  /* It's a raw value that needs special handling from the consumer */
	MONO_COUNTER_UNIT_BYTES, /* Quantity of bytes the counter represent */
	MONO_COUNTER_UNIT_TIME,  /* This is a timestap in 100n units */
	MONO_COUNTER_UNIT_EVENTS, /* Number of times the given event happens */
	MONO_COUNTER_UNIT_CONFIG, /* Configuration knob of the runtime */
	MONO_COUNTER_UNIT_PERCENTAGE, /* Percentage of something */

	MONO_COUNTER_UNIT_MAX
} MonoCounterUnit;

typedef enum {
	MONO_COUNTER_CONSTANT = 1, /* This counter doesn't change. Agent will only send it once */
	MONO_COUNTER_MONOTONIC, /* This counter value always increase/decreate over time */
	MONO_COUNTER_VARIABLE, /* This counter value can be anything on each sampling */

	MONO_COUNTER_VARIANCE_MAX
} MonoCounterVariance;

typedef struct _MonoCounter MonoCounter;

struct _MonoCounter {
	MonoCounter *next;
	const char *name;
	void *addr;
	MonoCounterType type;
	MonoCounterCategory category;
	MonoCounterUnit unit;
	MonoCounterVariance variance;
	gboolean is_callback;
};

/*
Limitations:
	The old-style string counter type won't work as they cannot be safely sampled during execution.

TODO:
	Size-bounded String counter.
	Sampler function that take user data arguments (could we use them for user perf counters?)
	Dynamic category registration.
	MonoCounter size diet once we're done with the above.
*/
void* mono_counters_new (MonoCounterCategory category, const char *name, MonoCounterType type, MonoCounterUnit unit, MonoCounterVariance variance) MONO_INTERNAL;
MonoCounter* mono_counters_register_full (MonoCounterCategory category, const char *name, MonoCounterType type, MonoCounterUnit unit, MonoCounterVariance variance, void *addr) MONO_INTERNAL;

MonoCounter* mono_counters_get (MonoCounterCategory category, const char* name) MONO_INTERNAL;
int mono_counters_sample (MonoCounter* counter, char* buffer, int size) MONO_INTERNAL;
int mono_counters_size   (MonoCounter* counter) MONO_INTERNAL;

MonoCounterCategory mono_counters_category_name_to_id (const char* name) MONO_INTERNAL;
const char* mono_counters_category_id_to_name (MonoCounterCategory id) MONO_INTERNAL;


#define mono_counters_new_int(cat,name,unit,variance) mono_counters_new(cat,name,MONO_COUNTER_TYPE_INT,unit,variance)
#define mono_counters_new_word(cat,name,unit,variance) mono_counters_new(cat,name,MONO_COUNTER_TYPE_WORD,unit,variance)
#define mono_counters_new_long(cat,name,unit,variance) mono_counters_new(cat,name,MONO_COUNTER_TYPE_LONG,unit,variance)
#define mono_counters_new_double(cat,name,unit,variance) mono_counters_new(cat,name,MONO_COUNTER_TYPE_double,unit,variance)

#define mono_counters_new_int_const(cat,name,unit,value) do { int *__ptr = mono_counters_new(cat,name,MONO_COUNTER_TYPE_INT,unit,variance); *__ptr = value; } while (0)
#define mono_counters_new_word_const(cat,name,unit,value) do { ssize_t *__ptr = mono_counters_new(cat,name,MONO_COUNTER_TYPE_INT,unit,variance); *__ptr = value; } while (0)
#define mono_counters_new_long_const(cat,name,unit,value) do { gint64 *__ptr = mono_counters_new(cat,name,MONO_COUNTER_TYPE_INT,unit,variance); *__ptr = value; } while (0)
#define mono_counters_new_double_const(cat,name,unit,value) do { double *__ptr = mono_counters_new(cat,name,MONO_COUNTER_TYPE_INT,unit,variance); *__ptr = value; } while (0)

#define mono_counters_inc(counter_ptr) do { *(counter_ptr) += 1; } while(0)
#define mono_counters_dec(counter_ptr) do { *(counter_ptr) += -1; } while(0)
#define mono_counters_add(counter_ptr, val) do { *(counter_ptr) += val; } while(0)
#define mono_counters_set(counter_ptr, val) do { *(counter_ptr) = val; } while(0)

#endif
