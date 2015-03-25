/*
 * main_mt.c
 *
 *  Created on: 20 марта 2015 г.
 *      Author: Andrey Perepelitsyn
 */

#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include "primes.h"

#define MAX_WORKERS 100

typedef struct PrimesWorkerInfo {
	primes_t range_start, range_end;
	int worker_id;
	pthread_t tid;
} primes_worker_t;

primes_worker_t workers[MAX_WORKERS];
primes_t
	range_start = 1,
	range_end = 42949672U, //4294967295U,
	range_chunk = 1000000U,
	current_start = 1;
primes_list_t *dividers, *results = NULL;
unsigned workers_count = 4, workers_done = 0;

pthread_mutex_t range_mutex, results_mutex;

primes_t
	base_dividers_data[] = {
		  2,  3,  5,  7, 11, 13, 17, 19, 23, 29,
		 31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
		 73, 79, 83, 89, 97,101,103,107,109,113,
		127,131,137,139,149,151,157,163,167,173,
		179,181,191,193,197,199,211,223,227,229,
		233,239,241,251 };

void *worker_func(void *thread_arg) {
	primes_t worker_start, worker_end;
	primes_list_t *result, *cur;
	primes_worker_t *info = (primes_worker_t *)thread_arg;

	while(current_start < range_end) {
		// new range for processing starts from 'current_start'
		// it's global, so we need to block this operation:
		assert(pthread_mutex_lock(&range_mutex) == 0);
		worker_start = current_start;
		current_start += range_chunk;
		assert(pthread_mutex_unlock(&range_mutex) == 0);

		if(worker_start > range_end)
			break;
		if(worker_start + range_chunk > range_end)
			worker_end = range_end;
		else
			worker_end = worker_start + range_chunk - 1;
		result = primes_calc(worker_start, worker_end, dividers);
		// adding new result block to results chain 'results'
		// it's global, so we need to block this operation:
		assert(pthread_mutex_lock(&results_mutex) == 0);
		if(results == NULL)
			results = result;
		else {
			cur = results;
			while(cur->next != NULL)
				cur = cur->next;
			cur->next = result;
		}
		assert(pthread_mutex_unlock(&results_mutex) == 0);
		printf("thread %d: chunk [%u, %u] calculated\n", info->worker_id,
				(unsigned)worker_start, (unsigned)worker_end);
	}
	// increment 'workers_done'
	assert(pthread_mutex_lock(&results_mutex) == 0);
	workers_done++;
	assert(pthread_mutex_unlock(&results_mutex) == 0);

	printf("thread %d: exit\n", info->worker_id);
	pthread_exit(NULL);
	return NULL;
}

int save_result_binary(FILE *f, primes_list_t *primes) {
	if(fwrite(primes->data, primes->count * sizeof(primes_t), 1, f) != 1) {
		perror("unable to write whole block:");
		return -1;
	}
	return 0;
}

int save_result_text(FILE *f, primes_list_t *primes) {
	primes_t i;
	for(i = 0; i < primes->count; i++)
		if(fprintf(f, "%u\n", (unsigned)primes->data[i]) <= 0) {
			perror("unable to write number to file:");
			return -1;
		}
	return 0;
}

int main(int argc,char *argv[])
{
	primes_list_t base_dividers, *cur, *prev;
	primes_t n, total;
	unsigned i;
	int rc;
	time_t time_elapsed;
	FILE *f;
	int (*save_result)(FILE *, primes_list_t *);

	time_elapsed = time(NULL);

	// first, calculate dividers for main calculation task
	printf("calculating dividers...");
	base_dividers.count = 54;
	base_dividers.data = base_dividers_data;
	base_dividers.next = NULL;
	dividers = primes_calc(1, 65535, &base_dividers);
	assert(dividers != NULL);
	printf("done.\ncalculating primes from range [%u, %u] in %u threads.\n",
			(unsigned)range_start, (unsigned)range_end, workers_count);

	// init mutexes and start worker threads
	assert(pthread_mutex_init(&range_mutex, NULL) == 0);
	assert(pthread_mutex_init(&results_mutex, NULL) == 0);
	for(i = 0; i < workers_count; i++) {
		workers[i].worker_id = i;
		rc = pthread_create(&workers[i].tid, NULL, worker_func, (void *)&workers[i]);
		if(rc) {
			fprintf(stderr, "pthread_create failed with code 0x%0x\n", rc);
			exit(-1);
		}
	}

	f = fopen("primes.dat", "wb");
	if(f == NULL) {
		perror("unable to open results file\n");
		exit(-1);
	}
	// now wait for results, save them to file and free memory
	//save_result = save_result_binary;
	save_result = save_result_text;
	n = range_start;
	total = 0;
	while(workers_done < workers_count || results != NULL) {
		// get blocks in right order
		// do, till appropriate blocks are found
		do {
			assert(pthread_mutex_lock(&results_mutex) == 0);
			cur = results;
			prev = NULL;
			while(cur != NULL && cur->range_start != n) {
				prev = cur;
				cur = cur->next;
			}
			if(cur != NULL) {
				// found right block, let's cut it from results chain
				if(prev != NULL)
					prev->next = cur->next;
				else
					results = cur->next;
				cur->next = NULL;
			}
			assert(pthread_mutex_unlock(&results_mutex) == 0);
			if(cur != NULL) {
				printf("saving [%u, %u]...\n", (unsigned)cur->range_start, (unsigned)cur->range_end);
				save_result(f, cur);
				n += range_chunk;
				total += cur->count;
				free(cur->data);
				free(cur);
			}
		} while(cur != NULL);
		sleep(1);
	}

	// all results are saved, let's finish gracefully
	for(i = 0; i < workers_count; i++) {
		assert(pthread_join(workers[i].tid, NULL) == 0);
		printf("thread %d joined.\n", i);
	}
	time_elapsed = time(NULL) - time_elapsed;
	printf("%u seconds elapsed, %u prime numbers saved\n",
			(unsigned)time_elapsed, (unsigned)total);
	fclose(f);
	return 0;
}

