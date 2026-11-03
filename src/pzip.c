#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pzip.h"

/**
 * struct shared_state - Data shared by every worker thread
 *
 * @input_chars:	The input characters (a-z) to be zipped
 * @chunk_size:		Number of input characters each thread processes
 * @n_threads:		The number of worker threads
 * @zipped_chars:	The output array of zipped_char structs
 * @zipped_chars_count:	The output count of zipped_char structs
 * @char_frequency:	The output array of total occurrences per letter
 * @local_counts:	Number of zipped_char structs found by each thread
 * @freq_lock:		Protects @char_frequency
 * @barrier:		Waits until every thread has finished counting
 */
struct shared_state {
	const char *input_chars;
	int chunk_size;
	int n_threads;
	struct zipped_char *zipped_chars;
	int *zipped_chars_count;
	int *char_frequency;
	int *local_counts;
	pthread_mutex_t freq_lock;
	pthread_barrier_t barrier;
};

/**
 * struct thread_arg - Per-thread argument passed to pzip_worker()
 *
 * @shared:	The state shared by all threads
 * @id:		Index of this thread, which selects its input chunk
 */
struct thread_arg {
	struct shared_state *shared;
	int id;
};

static void die(const char *msg, int err)
{
	fprintf(stderr, "Error: %s: %s\n", msg, strerror(err));
	exit(EXIT_FAILURE);
}

/**
 * pzip_worker() - zip one chunk of the input
 *
 * Counts consecutive runs in this thread's chunk into a local buffer, waits
 * for every other thread to do the same, then copies its local results into
 * the correct position of the shared zipped_chars array.
 */
static void *pzip_worker(void *arg)
{
	struct thread_arg *targ = arg;
	struct shared_state *s = targ->shared;
	int id = targ->id;
	const char *start = s->input_chars + (size_t)id * s->chunk_size;
	const char *end = start + s->chunk_size;
	int local_freq[26] = { 0 };
	struct zipped_char *local_result;
	int count = 0;
	int offset = 0;
	int err;

	local_result = malloc(s->chunk_size * sizeof(*local_result));
	if (!local_result)
		die("malloc", ENOMEM);

	/* Step 1: count consecutive occurrences in this thread's chunk */
	for (const char *p = start; p < end;) {
		char c = *p;
		const char *run = p;

		while (p < end && *p == c)
			p++;
		local_result[count].character = c;
		local_result[count].occurence = (uint8_t)(p - run);
		local_freq[c - 'a'] += p - run;
		count++;
	}
	s->local_counts[id] = count;

	pthread_mutex_lock(&s->freq_lock);
	for (int i = 0; i < 26; i++)
		s->char_frequency[i] += local_freq[i];
	pthread_mutex_unlock(&s->freq_lock);

	/* Step 2: wait until every thread knows its local count */
	err = pthread_barrier_wait(&s->barrier);
	if (err && err != PTHREAD_BARRIER_SERIAL_THREAD)
		die("pthread_barrier_wait", err);

	for (int i = 0; i < id; i++)
		offset += s->local_counts[i];

	memcpy(s->zipped_chars + offset, local_result,
	       count * sizeof(*local_result));

	if (id == s->n_threads - 1)
		*s->zipped_chars_count = offset + count;

	free(local_result);
	return NULL;
}

/**
 * pzip() - zip an array of characters in parallel
 *
 * Inputs:
 * @n_threads:		   The number of threads to use in pzip
 * @input_chars:		   The input characters (a-z) to be zipped
 * @input_chars_size:	   The number of characaters in the input file
 *
 * Outputs:
 * @zipped_chars:       The array of zipped_char structs
 * @zipped_chars_count:   The total count of inserted elements into the zipped_chars array
 * @char_frequency: Total number of occurences
 *
 * NOTE: All outputs are already allocated. DO NOT MALLOC or REASSIGN THEM !!!
 *
 */
void pzip(int n_threads, char *input_chars, int input_chars_size,
	  struct zipped_char *zipped_chars, int *zipped_chars_count,
	  int *char_frequency)
{
	struct shared_state s = {
		.input_chars = input_chars,
		.chunk_size = input_chars_size / n_threads,
		.n_threads = n_threads,
		.zipped_chars = zipped_chars,
		.zipped_chars_count = zipped_chars_count,
		.char_frequency = char_frequency,
	};
	pthread_t *threads;
	struct thread_arg *args;
	int err;

	threads = malloc(n_threads * sizeof(*threads));
	args = malloc(n_threads * sizeof(*args));
	s.local_counts = calloc(n_threads, sizeof(*s.local_counts));
	if (!threads || !args || !s.local_counts)
		die("malloc", ENOMEM);

	err = pthread_mutex_init(&s.freq_lock, NULL);
	if (err)
		die("pthread_mutex_init", err);
	err = pthread_barrier_init(&s.barrier, NULL, n_threads);
	if (err)
		die("pthread_barrier_init", err);

	for (int i = 0; i < n_threads; i++) {
		args[i].shared = &s;
		args[i].id = i;
		err = pthread_create(&threads[i], NULL, pzip_worker, &args[i]);
		if (err)
			die("pthread_create", err);
	}

	for (int i = 0; i < n_threads; i++) {
		err = pthread_join(threads[i], NULL);
		if (err)
			die("pthread_join", err);
	}

	pthread_barrier_destroy(&s.barrier);
	pthread_mutex_destroy(&s.freq_lock);
	free(s.local_counts);
	free(args);
	free(threads);
}
