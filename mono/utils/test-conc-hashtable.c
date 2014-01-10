/*
 * test-conc-hashtable.c: Unit test for the concurrent hashtable.
 *
 * Copyright (C) 2013 Xamarin Inc
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License 2.0 as published by the Free Software Foundation;
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License 2.0 along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"

#include "utils/mono-threads.h"
#include "utils/mono-conc-hashtable.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include <pthread.h>

static int
serial_test (void)
{
	mono_mutex_t mutex;
	MonoConcurrentHashTable *h;
	int res = 0;

	mono_mutex_init (&mutex);
	h = mono_conc_hashtable_new (&mutex, NULL, NULL);
	mono_conc_hashtable_insert (h, GUINT_TO_POINTER (10), GUINT_TO_POINTER (20));
	mono_conc_hashtable_insert (h, GUINT_TO_POINTER (30), GUINT_TO_POINTER (40));
	mono_conc_hashtable_insert (h, GUINT_TO_POINTER (50), GUINT_TO_POINTER (60));
	mono_conc_hashtable_insert (h, GUINT_TO_POINTER (2), GUINT_TO_POINTER (3));

	if (mono_conc_hashtable_lookup (h, GUINT_TO_POINTER (30)) != GUINT_TO_POINTER (40))
		res = 1;
	if (mono_conc_hashtable_lookup (h, GUINT_TO_POINTER (10)) != GUINT_TO_POINTER (20))
		res = 2;
	if (mono_conc_hashtable_lookup (h, GUINT_TO_POINTER (2)) != GUINT_TO_POINTER (3))
		res = 3;
	if (mono_conc_hashtable_lookup (h, GUINT_TO_POINTER (50)) != GUINT_TO_POINTER (60))
		res = 4;

	mono_conc_hashtable_destroy (h);
	mono_mutex_destroy (&mutex);
	if (res)
		printf ("SERIAL TEST FAILED %d\n", res);
	return res;
}

static MonoConcurrentHashTable *hash;

static void*
pw_sr_thread (void *arg)
{
	int i, idx = 1000 * GPOINTER_TO_INT (arg);
	mono_thread_info_attach ((gpointer)&arg);

	for (i = 0; i < 1000; ++i)
		mono_conc_hashtable_insert (hash, GINT_TO_POINTER (i + idx), GINT_TO_POINTER (i));
	return NULL;
}

static int
parallel_writers_single_reader (void)
{
	pthread_t a,b,c;
	mono_mutex_t mutex;
	int i, j, res = 0;

	mono_mutex_init (&mutex);
	hash = mono_conc_hashtable_new (&mutex, NULL, NULL);

	pthread_create (&a, NULL, pw_sr_thread, GINT_TO_POINTER (0));
	pthread_create (&b, NULL, pw_sr_thread, GINT_TO_POINTER (1));
	pthread_create (&c, NULL, pw_sr_thread, GINT_TO_POINTER (2));

	pthread_join (a, NULL);
	pthread_join (b, NULL);
	pthread_join (c, NULL);

	for (i = 0; i < 1000; ++i) {
		for (j = 0; j < 3; ++j) {
			if (mono_conc_hashtable_lookup (hash, GINT_TO_POINTER (j * 1000 + i)) != GINT_TO_POINTER (i)) {
				res = j + 1;
				goto done;
			}
		}
	}

done:
	mono_conc_hashtable_destroy (hash);
	mono_mutex_destroy (&mutex);
	if (res)
		printf ("PAR_WRITER_SINGLE_READER TEST FAILED %d\n", res);
	return res;
}


static void*
pw_sw_thread (void *arg)
{
	int i = 0, idx = 100 * GPOINTER_TO_INT (arg);
	mono_thread_info_attach ((gpointer)&arg);

	while (i < 100) {
		gpointer res = mono_conc_hashtable_lookup (hash, GINT_TO_POINTER (i + idx + 1));
		if (!res)
			continue;
		if (res != GINT_TO_POINTER ((i + idx) * 2 + 1))
			return GINT_TO_POINTER (i);
		++i;
	}
	return NULL;
}

static int
parallel_reader_single_writer (void)
{
	pthread_t a,b,c;
	mono_mutex_t mutex;
	gpointer ra, rb, rc;
	int i, res = 0;
	ra = rb = rc = GINT_TO_POINTER (1);

	mono_mutex_init (&mutex);
	hash = mono_conc_hashtable_new (&mutex, NULL, NULL);

	pthread_create (&a, NULL, pw_sw_thread, GINT_TO_POINTER (0));
	pthread_create (&b, NULL, pw_sw_thread, GINT_TO_POINTER (1));
	pthread_create (&c, NULL, pw_sw_thread, GINT_TO_POINTER (2));

	for (i = 0; i < 100; ++i) {
		mono_conc_hashtable_insert (hash, GINT_TO_POINTER (i +   0 + 1), GINT_TO_POINTER ((i +   0) * 2 + 1));
		mono_conc_hashtable_insert (hash, GINT_TO_POINTER (i + 100 + 1), GINT_TO_POINTER ((i + 100) * 2 + 1));
		mono_conc_hashtable_insert (hash, GINT_TO_POINTER (i + 200 + 1), GINT_TO_POINTER ((i + 200) * 2 + 1));
	}

	pthread_join (a, &ra);
	pthread_join (b, &rb);
	pthread_join (c, &rc);
	res = GPOINTER_TO_INT (ra) + GPOINTER_TO_INT (rb) + GPOINTER_TO_INT (rc);

	mono_conc_hashtable_destroy (hash);
	mono_mutex_destroy (&mutex);
	if (res)
		printf ("SINGLE_WRITER_PAR_READER TEST FAILED %d\n", res);
	return res;
}

static void
benchmark_conc (void)
{
	mono_mutex_t mutex;
	MonoConcurrentHashTable *h;
	int i, j;

	mono_mutex_init (&mutex);
	h = mono_conc_hashtable_new (&mutex, NULL, NULL);

	for (i = 1; i < 10 * 1000; ++i)
		mono_conc_hashtable_insert (h, GUINT_TO_POINTER (i), GUINT_TO_POINTER (i));


	for (j = 0; j < 100000; ++j)
		for (i = 1; i < 10 * 105; ++i)
			mono_conc_hashtable_lookup (h, GUINT_TO_POINTER (i));

	mono_conc_hashtable_destroy (h);
	mono_mutex_destroy (&mutex);

}

static void
benchmark_glib (void)
{
	GHashTable *h;
	int i, j;

	h = g_hash_table_new (NULL, NULL);

	for (i = 1; i < 10 * 1000; ++i)
		g_hash_table_insert (h, GUINT_TO_POINTER (i), GUINT_TO_POINTER (i));


	for (j = 0; j < 100000; ++j)
		for (i = 1; i < 10 * 105; ++i)
			g_hash_table_lookup (h, GUINT_TO_POINTER (i));

	g_hash_table_destroy (h);
}

int
main (void)
{
	MonoThreadInfoCallbacks cb = { NULL };
	int res = 0;

	mono_threads_init (&cb, sizeof (MonoThreadInfo));
	mono_thread_info_attach ((gpointer)&cb);

	benchmark_conc ();
	//benchmark_glib ();
	// res |= serial_test ();
	// res |= parallel_writers_single_reader ();
	// res |= parallel_reader_single_writer ();
	return res;
}
