/*
 * main_mt.c
 *
 *  Created on: 20 марта 2015 г.
 *      Author: Andrey Perepelitsyn
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
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
	range_end = 40000000U,
	range_chunk = 1000000U,
	current_start = 1;
primes_list_t *dividers, *results = NULL;

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
		// критическая секция: берем задание на обработку
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
		printf("thread %d: calculating range [%u, %u]\n", info->worker_id,
				(unsigned)worker_start, (unsigned)worker_end);
		result = primes_calc(worker_start, worker_end, dividers);
		// критическая секция: готовый блок втыкаем в список результатов
		// несколько сотен проходов простенького цикла можно и потерпеть
		assert(pthread_mutex_lock(&results_mutex) == 0);
		if(results == NULL)
			results = result;
		else {
			if(result->range_end < results->range_start) {
				result->next = results;
				results = result;
			}
			else {
				cur = results;
				while(cur->next != NULL && result->range_start > cur->next->range_end)
					cur = cur->next;
				if(cur->next != NULL)
					result->next = cur->next;
				cur->next = result;
			}
		}
		assert(pthread_mutex_unlock(&results_mutex) == 0);
	}
	printf("thread %d: exit\n", info->worker_id);
	pthread_exit(NULL);
	return NULL;
}

int main(int argc,char *argv[])
{
	primes_list_t base_dividers, *cur;
	primes_t n;
	unsigned i, workers_count = 4;
	int rc;
	time_t time_elapsed;
	FILE *f;

	time_elapsed = time(NULL);
	base_dividers.count = 54;
	base_dividers.data = base_dividers_data;
	base_dividers.next = NULL;
	dividers = primes_calc(1, 65535, &base_dividers);
	assert(dividers != NULL);
	printf("%u primes found.\n", (unsigned)dividers->count);

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
	for(i = 0; i < workers_count; i++) {
		printf("waiting for thread %d...\n", i);
		assert(pthread_join(workers[i].tid, NULL) == 0);
		printf("thread %d joined.\n", i);
	}
	time_elapsed = time(NULL) - time_elapsed;
	printf("%u seconds elapsed\nresults:\n", (unsigned)time_elapsed);
	f = fopen("primes.dat", "wb");
	if(f == NULL) {
		perror("unable to open results file\n");
		exit(-1);
	}
	cur = results;
	n = 0;
	while(cur != NULL) {
		printf("[%u, %u]\n", (unsigned)cur->range_start, (unsigned)cur->range_end);
		if(fwrite(cur->data, cur->count * sizeof(primes_t), 1, f) != 1) {
			perror("unable to write whole block:");
			exit(-1);
		}
		n += cur->count;
		cur = cur->next;
	}
	fclose(f);
	printf("%u primes found and saved\n", (unsigned)n);
	return 0;
}

