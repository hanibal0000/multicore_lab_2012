#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include "array.h"

// To be defined while calling compiler
// #define NB_THREADS 3

struct thread_res
{
	int sum;
	struct timespec start, stop;
};
typedef struct thread_res thread_res_t;

struct thread_arg
{
	int id;
	struct array * array;
	thread_res_t res;
};
typedef struct thread_arg thread_arg_t;

int prefix_sum(struct array * array, int start, int stop)
{
	// Declarations
	int i, res;

	// Initialise summation counter to 0
	res = 0;

	// Iterates through all values and sum it up to res
	for(i = start; i < stop; i++)
	{
		res += array->data[i];
	}

	// Return result
	return res;
}

void * thread_work(void* arg)
{
	thread_arg_t* args = (thread_arg_t*)arg;
	int block_size, extra;

#if NB_THREADS == 0
	block_size = args->array->length;
	extra = 0;
#else
	block_size = args->array->length / NB_THREADS;
	extra = args->id < NB_THREADS - 1 ? 0 : args->array->length % NB_THREADS;
#endif

	//res = malloc(sizeof(thread_res_t));
	
	clock_gettime(CLOCK_MONOTONIC, &args->res.start);
	args->res.sum = prefix_sum(args->array, args->id * block_size, (args->id + 1) * block_size + extra);
	clock_gettime(CLOCK_MONOTONIC, &args->res.stop);
	
	return NULL;
}

int
main(int argc, char** argv)
{
  // Declarations
  struct array * array;
  struct timespec start, stop;

  // Load input data from file
  array = array_read(argv[1]);

#if NB_THREADS == 0
  // Start measurement
  clock_gettime(CLOCK_MONOTONIC, &start);

  prefix_sum(array, 0, array->length);

  // Stop measurement
  clock_gettime(CLOCK_MONOTONIC, &stop);

  printf("0 %i %li %i %li %i %li %i %li\n", (int)start.tv_sec, start.tv_nsec, (int)stop.tv_sec, stop.tv_nsec, (int)start.tv_sec, start.tv_nsec, (int)stop.tv_sec, stop.tv_nsec);
#else
  int i, sum;

  thread_arg_t args[NB_THREADS];
  thread_res_t res[NB_THREADS], *res_ptr;
  // pthread management
  pthread_t thread[NB_THREADS];
  pthread_attr_t thread_attr;

  // Initialize attributes for threads
  pthread_attr_init(&thread_attr);

  // Start measurement
  clock_gettime(CLOCK_MONOTONIC, &start);

  for(i = 0; i < NB_THREADS; i++)
  {
    args[i].id = i;
    args[i].array = array;

    pthread_create(&thread[i], &thread_attr, &thread_work, &args[i]);
  }

  sum = 0;
  for (i = 0; i < NB_THREADS; i++)
  {
    res_ptr = &res[i];
    pthread_join(thread[i], NULL/*(void**)&res_ptr*/);
    sum += res_ptr->sum;
  }

  // Stop measurement
  clock_gettime(CLOCK_MONOTONIC, &stop);

  // Output result with measured timing
  for (i = 0; i < NB_THREADS; i++)
  {
    printf("%i %i %li %i %li %i %li %i %li\n", i, (int)start.tv_sec, start.tv_nsec, (int)stop.tv_sec, stop.tv_nsec, (int)args[i].res.start.tv_sec, args[i].res.start.tv_nsec, (int)args[i].res.stop.tv_sec, args[i].res.stop.tv_nsec);
  }
#endif

  // Free memory out
  array_free(array);

  // Everything went well
  return 0;
}
