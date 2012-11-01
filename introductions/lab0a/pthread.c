/*
 * pthread.c
 *
 *  Created on: 5 Sep 2011
 *  Copyright 2011 Nicolas Melot
 *
 * This file is part of TDDD56.
 * 
 *     TDDD56 is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 * 
 *     TDDD56 is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 * 
 *     You should have received a copy of the GNU General Public License
 *     along with TDDD56. If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

inline volatile long long
rdtsc()
{
	register long long tsc asm("eax");
	asm volatile (".byte 15, 49" : : : "eax", "edx");

	return tsc;
} 

#define NB_THREADS 3

struct thread_arg
{
	int id;
	long long int spawn_time;
};
typedef struct thread_arg thread_arg_t;

struct thread_res
{
	long long int finish_time;
};
typedef struct thread_res thread_res_t;

void *
thread_work(void * arg)
{
	thread_res_t * res;
	thread_arg_t * args = (thread_arg_t*)arg;

	res = malloc(sizeof(thread_res_t));

	printf("[THREAD %d] Spawned at %Li\n", args->id, args->spawn_time);
	res->finish_time = rdtsc();

	pthread_exit(res);
}

int
main(int argc, char ** argv)
{
	int i;

	thread_res_t *res;
	thread_arg_t args[NB_THREADS];
	pthread_t thread[NB_THREADS];
	pthread_attr_t thread_attr;

	// Initialize attributes for threads
	pthread_attr_init(&thread_attr);

	for(i = 0; i < NB_THREADS; i++)
	{
		args[i].id = i;
		args[i].spawn_time = rdtsc();

		pthread_create(&thread[i], &thread_attr, &thread_work, &args[i]);
	}

	for(i = 0; i < NB_THREADS; i++)
	{
		pthread_join(thread[i], (void**)&res);
		printf("Thread %d terminated at %Li\n", args[i].id, res->finish_time);
		free(res);
	}

	return EXIT_SUCCESS;
}
