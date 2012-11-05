#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <malloc.h>

#include "mandelbrot.h"
#include "ppm.h"

#ifdef MEASURE
#include <time.h>
// If we measure, we don't debug as assert() and printf() seriously affect performance
#undef DEBUG
#endif

// Disable assertion code if DEBUG is not defined
#ifndef DEBUG
#define NDEBUG
#endif

#if NB_THREADS > 0
#include <pthread.h>
#endif

#if LOADBALANCE == 2
#include<math.h>
#endif

int * color = NULL;

#if NB_THREADS > 0
// Compiled only when several threads are used
struct mandelbrot_thread
{
	int id;
#ifdef MEASURE
struct mandelbrot_timing timing;
#endif
};

int thread_stop;
pthread_barrier_t thread_pool_barrier;

pthread_t thread[NB_THREADS];
struct mandelbrot_thread thread_data[NB_THREADS];

#if LOADBALANCE == 2
//representing for every thread what his work is and what he has done
struct thread_workload {
    int start;
    int end;
    int pos;
};

//storing work status for every thread
struct thread_workload workloads[NB_THREADS];

//mutex for workload access
pthread_mutex_t mutex[NB_THREADS];
#endif

#else
#ifdef MEASURE
struct mandelbrot_timing sequential;
#endif
#endif

#ifdef MEASURE
struct mandelbrot_timing **timing;
#endif

struct mandelbrot_param mandelbrot_param;

static int num_colors(struct mandelbrot_param* param)
{
	return param->maxiter + 1;
}

/**
 * Calculates if the complex number (Cre, Cim)
 * belongs to the Mandelbrot set
 *
 * @param Cre: Real part
 *
 * @param Cim: Imaginary part
 *
 * @return : MAXITER if (Cre, Cim) belong to the
 * mandelbrot set, else the number of iterations
 */
static int
is_in_Mandelbrot(float Cre, float Cim, int maxiter)
{
	int iter;
	float x = 0.0, y = 0.0, xto2 = 0.0, yto2 = 0.0, dist2;

	for (iter = 0; dist2 < 4 && iter <= maxiter; iter++)
	{
		y = x * y;
		y = y + y + Cim;
		x = xto2 - yto2 + Cre;
		xto2 = x * x;
		yto2 = y * y;

		dist2 = xto2 + yto2;
	}
	return iter;
}

/***** You may modify this portion *****/

#if LOADBALANCE == 2
/**
 *  looks if work for thread is finished or not
 */
int
has_work_todo(int thread_id)
{
    int ret;
    pthread_mutex_lock( &mutex[thread_id] );
    ret = workloads[thread_id].end != workloads[thread_id].pos;
    pthread_mutex_unlock( &mutex[thread_id] );
    return ret;
}

/**
 * Implementation of work stealing algorithm.
 * Always steals work from worst performing thread (thread with most work left)
 * Since we increment the position we are currently performing on before starting to perform
 * (see compute_chunk) we never compute stuff twice, since threads seems finished before being finished
 */
void
find_new_work(int thread_id)
{
    int worst_thread_id = -1;
    int worst_thread_lines_left=0;
    int i;
    for(i = 0; i < NB_THREADS; i++) {
        //skip own thread
        if(i == thread_id)
            continue;

        pthread_mutex_lock( &mutex[i] );
        int current_lines_left = workloads[i].end - workloads[i].pos;
        pthread_mutex_unlock( &mutex[i] );

        //new candidate to steal from?
        if(current_lines_left < 1)
            continue;
        else
            worst_thread_id = worst_thread_lines_left > current_lines_left ? worst_thread_id : i;
    }

    if(worst_thread_id == -1)
        return;

    //lock to change work
    pthread_mutex_lock( &mutex[worst_thread_id] );
    pthread_mutex_lock( &mutex[thread_id] );
    
    struct thread_workload *workload = &workloads[worst_thread_id];
    workloads[thread_id].end = workload->end;
    workloads[thread_id].start = workload->pos + floor((workload->end - workload->pos)/2);
    workloads[thread_id].pos = workloads[thread_id].start;
    workload->end = workloads[thread_id].start;

    //unlock
    pthread_mutex_unlock( &mutex[worst_thread_id] );
    pthread_mutex_unlock( &mutex[thread_id] );
}
#endif


#if LOADBALANCE == 2
static void
compute_chunk(int thread_id, struct mandelbrot_param *args)
#else
static void
compute_chunk(struct mandelbrot_param *args)
#endif
{
	int i, j, val;
	float Cim, Cre;
	color_t pixel;

#if LOADBALANCE == 2
    pthread_mutex_lock( &mutex[thread_id] );
    int end = workloads[thread_id].end;
    i = workloads[thread_id].pos;
    pthread_mutex_unlock( &mutex[thread_id] );
    
    while(i < end)
#else
	for (i = 0; i < args->height; i++)
#endif
	{
		for (j = 0; j < args->width; j++)
		{
			// Convert the coordinate of the pixel to be calculated to both
			// real and imaginary parts of the complex number to be checked
			Cim = (float) i / args->height * (args->upper_i - args->lower_i)
			    + args->lower_i;
			Cre = (float) j / args->width * (args->upper_r - args->lower_r)
			    + args->lower_r;

			// Gets the value returned by is_in_mandelbrot() and scale it
			// from 0 to 255, or -1 if (Cre, Cim) is in the mandelbrot set.
			val = is_in_Mandelbrot(Cre, Cim, args->maxiter);

			// Change a negative value to 0 in val to make mandelbrot
			// elements to appear black in the final picture.
			val = val > args->maxiter ? args->mandelbrot_color : color[val
			    % num_colors(args)];

			// Permute green, red and blue to get different fancy colors
			pixel.green = val >> 16 & 255;
			pixel.red = val >> 8 & 255;
			pixel.blue = val & 255;
			ppm_write(args->picture, j, i, pixel);
		}

#if LOADBALANCE == 2
        //increase position before mutexing
        i++;
        
        //mutexed access to shared workdata
        pthread_mutex_lock( &mutex[thread_id] );
        end = workloads[thread_id].end;
        workloads[thread_id].pos = i;
        pthread_mutex_unlock( &mutex[thread_id] );
#endif 
	}
}

#if LOADBALANCE == 2
void
init_round(struct mandelbrot_thread *args)
#else
void
init_round()
#endif
{
	// Initialize or reinitialize here variables before any thread starts or restarts computation
#if LOADBALANCE == 2    
    //initialize new mutex for new round (TODO can be left out??)
    pthread_mutex_destroy(&mutex[args->id]);
    pthread_mutex_init(&mutex[args->id], NULL);

    //lock access for reinitialization
    pthread_mutex_lock( &mutex[args->id] );

    int work_size = ceil(mandelbrot_param.height / NB_THREADS);
    workloads[args->id].start = args->id * work_size;
    int computed_end = args->id * work_size + work_size;
    workloads[args->id].end = computed_end < mandelbrot_param.height ? computed_end : mandelbrot_param.height;
    workloads[args->id].pos = workloads[args->id].start;
    
    //free access
    pthread_mutex_unlock( &mutex[args->id] );
#endif
}

#if NB_THREADS > 0
void
parallel_mandelbrot(struct mandelbrot_thread *args, struct mandelbrot_param *parameters)
{
#if LOADBALANCE == 2
    while(has_work_todo(args->id)) 
    {
        compute_chunk(args->id, parameters);

        find_new_work(args->id);
    }

#else
	if (args->id == 0)
	{
		compute_chunk(parameters);
	}
#endif
}
#else
void
sequential_mandelbrot(struct mandelbrot_param *parameters)
{
	compute_chunk(parameters);
}
#endif
/***** end *****/

// Thread code, compiled only if we use threads
#if NB_THREADS > 0
static void *
run_thread(void * buffer)
{
	struct mandelbrot_thread *args;
	args = (struct mandelbrot_thread*) buffer;

	// Notify the master this thread is spawned
	pthread_barrier_wait(&thread_pool_barrier);

	// Reinitialize vars before any thread restart
#if LOADBALANCE == 2
    init_round(args);
#else
	init_round();
#endif

	// Wait for the first computation order
	pthread_barrier_wait(&thread_pool_barrier);

	while (thread_stop == 0)
	{
#ifdef MEASURE
		clock_gettime(CLOCK_MONOTONIC, &args->timing.start);
#endif

	parallel_mandelbrot(args, &mandelbrot_param);

#ifdef MEASURE
		clock_gettime(CLOCK_MONOTONIC, &args->timing.stop);
#endif

		// Notify the master thread of completion
		pthread_barrier_wait(&thread_pool_barrier);

		// Reinitialize vars before any thread restart
#if LOADBALANCE == 2
        init_round(args);
#else
    	init_round();
#endif

		// Wait for the next work signal
		pthread_barrier_wait(&thread_pool_barrier);
	}

	// Notify the master thread of work completion
	pthread_barrier_wait(&thread_pool_barrier);

	return NULL;
}
#endif

void
init_ppm(struct mandelbrot_param* param)
{
	if(param->picture->data != NULL)
	{
		free(param->picture->data);
		param->picture->data = NULL;
	}

	param->picture->data = malloc(ppm_align(sizeof(color_t) * param->width, PPM_ALIGNMENT) * param->height);
	param->picture->height = param->height;
	param->picture->width = param->width;
}

void
update_colors(struct mandelbrot_param* param)
{
	// Gradient color
	int start_r, start_g, start_b, end_r, end_g, end_b;
	// Other control variables
	int i;

	if(color != NULL)
	{
		free(color);
	}
	color = malloc(sizeof(int) * num_colors(param));

	// Start color
	start_r = 219;
	start_g = 57;
	start_b = 0;

	// Stop color
	end_r = 0;
	end_g = 0;
	end_b = 0;

	// Initialize the color vector
	for (i = 0; i < num_colors(param); i++)
	{
		color[i]
		    = (((int) ((end_g - start_g) * ((double) i / num_colors(param)) + start_g)
		        & 255) << 16) + (((int) ((end_r - start_r) * ((double) i
		        / num_colors(param)) + start_r) & 255) << 8) + (((int) ((end_b - start_b)
		        * ((double) i / num_colors(param)) + start_b) & 255) << 0);
	}
}

void
init_mandelbrot(struct mandelbrot_param *param)
{
	// Initialize the picture container, but not its buffer
	param->picture = ppm_alloc(0, 0);
	param->picture->height = param->height;
	param->picture->width = param->width;

#if GLUT != 1
	// GLUT will do it when creating or resizing its window
	init_ppm(param);
#endif
	// initialize the color vector
	update_colors(param);

#if NB_THREADS > 0
	// Thread-based variant

	pthread_attr_t thread_attr;
	int i;

	// Initialise thread poll / master thread synchronisation
	pthread_barrier_init(&thread_pool_barrier, NULL, NB_THREADS + 1);

	// Initialize attributes
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);

	// Enables thread running
	thread_stop = 0;

#ifdef MEASURE
	// Measuring record structures
	timing = malloc(sizeof(struct timing*) * NB_THREADS);
#endif

	// Create a thread pool
	for (i = 0; i < NB_THREADS; i++)
	{
		thread_data[i].id = i;

#ifdef MEASURE
		timing[i] = &thread_data[i].timing;
#endif

		// Check the good behavior or pthread_create; must be disabled while measuring for performance reasons
#ifdef DEBUG
		assert(pthread_create(&thread[i], &thread_attr, &run_thread, &thread_data[i]) == 0);
#else
		pthread_create(&thread[i], &thread_attr, &run_thread, &thread_data[i]);
#endif
	}

	// Wait for the thread to be fully spawned before returning
	pthread_barrier_wait(&thread_pool_barrier);
#else
#ifdef MEASURE
	// Measuring record structures
	timing = malloc(sizeof(struct timing*));
	timing[0] = &sequential;
#endif
#endif
}

#ifdef MEASURE
struct mandelbrot_timing**
#else
void
#endif
compute_mandelbrot(struct mandelbrot_param param)
{
#if NB_THREADS > 0
	mandelbrot_param = param;

	// Trigger threads' resume
	pthread_barrier_wait(&thread_pool_barrier);

	// Wait for the threads to be done
	pthread_barrier_wait(&thread_pool_barrier);
#else
#ifdef MEASURE
	clock_gettime(CLOCK_MONOTONIC, &sequential.start);
#endif

	init_round();
	sequential_mandelbrot(&param);

#ifdef MEASURE
	clock_gettime(CLOCK_MONOTONIC, &sequential.stop);
#endif
#endif

#ifdef MEASURE
	return timing;
#endif
}

void
destroy_mandelbrot(struct mandelbrot_param param)
{
#if NB_THREADS > 0
	int i;

	// Initiate a stop order and resume threads in the thread pool
	thread_stop = 1;
	compute_mandelbrot(param);

	// Wait for the threads to finish
	for (i = 0; i < NB_THREADS; i++)
	{
#ifdef DEBUG
		assert(pthread_join(thread[i], NULL) == 0);
#else
		pthread_join(thread[i], NULL);
#endif
	}

	pthread_barrier_destroy(&thread_pool_barrier);

#else
	// Sequential version, nothing to destroy
#endif

#ifdef MEASURE
	free(timing);
#endif

	free(color);
	ppm_free(param.picture);
}
