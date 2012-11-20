/*
 * sort.c
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

// Do not touch or move these lines
#include <stdio.h>
#include "disable.h"

#ifndef DEBUG
#define NDEBUG
#endif

#include "array.h"
#include "sort.h"
#include "simple_quicksort.h"

#if NB_THREADS > 0
#include <pthread.h>
#include <math.h>
#include <stdlib.h>
#endif

/* defines */

#define L1_CACH_FALLBACK 14000

typedef struct array array_t;
struct par_qsrt_thread
{
    int id;
}

typedef struct seq_work {
    array_t* to_sort;
    int sort_from;
    int sort_to;
    int global_start_pos;
} seq_work_t;

typedef struct recursion_tree_node {
    struct array* to_sort;
    int sort_from;
    int sort_to;

    struct array* partitioned;
    int pivot_pos;

    struct recursion_tree_node* parent;
    struct recursion_tree_node* left;
    struct recursion_tree_node* right;

    int global_pos;

    stack_t *seq_work;

    int stored_seq_work;
    int initialized_for_partitioning;
    int created_left_child;
    int created_right_child;
    int initialized_pivot_sort;
} recursion_tree_node_t;


pthread_t thread[NB_THREADS];
struct par_qsrt_thread thread_data[NB_THREADS];

pthread_mutex_t work_adder_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t temp_array_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pivot_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t sync_barrier = PTHREAD_BARRIER_INITIALIZER(NB_THREADS);

int left, right;

struct stack_t* seq_work_stack;
struct recursive_tree_node* current_pos;

struct array *random_set;
int picked;

/* code */

/************************************************
 *          sequential algorithms                *
 ************************************************/

    void
merge(int low, int middle, int high, struct array* helperArray, struct array* array)
{
    int i  = 0;
    for (  i = low; i <=high; i++)
    {
        array_insert(helperArray,array->data[i],i);
    }

    i = low;
    int j = middle +1;
    int k = low;

    while (i <= middle && j <= high) 
    {
        if (helperArray->data[i] <= helperArray->data[j])
        {
            array_insert(array,helperArray->data[i],k);
            i++;
        } else 
        {
            array_insert(array,helperArray->data[j],k);
            j++;
        }
        k++;
    }
    while (i <= middle) {
        array_insert(array,helperArray->data[i],k);
        k++;
        i++;
    } 

    while (j <= high) {
        array_insert(array,helperArray->data[j],k);
        k++;
        j++;
    }
}

    void
mergeSort(int low, int high, struct array* helperArray, struct array* array)
{
    if( low < high)
    {
        int middle = (int) ((low + high) / 2) ;
        //printf("Splitting with low=%d\t middle=%d\t high = %d\n",low, middle, high);
        mergeSort(low, middle,helperArray,array);
        mergeSort(middle+1,high,helperArray,array);
        merge(low, middle, high, helperArray, array);
    }
}

    void 
quicksort(int low, int high, struct array * array)
{
    //TODO insert treshold for fallback
    // choose a good pivot element
    int i = 0;
    int pivotElement = 0;
    int pivotChoice = (int) sqrt(high-low);
    srand(pivotChoice);
    struct array *pivotElements  = array_alloc(pivotChoice); 
    struct array* helperArray = array_alloc(pivotChoice);
    for ( i = 0; i < pivotChoice; i++)
    {
        array_put(pivotElements,array->data[low+rand()%pivotChoice]);
        array_put(helperArray,pivotElements->data[i]);
    }
    mergeSort(0, pivotChoice-1, helperArray, pivotElements);
    // choose median
    pivotElement = pivotElements->data[(int)(pivotChoice/2)];


    //do quicksort
    i = low;
    int j = high;

    while( i<= j)
    {
        while(array->data[i] < pivotElement)
        {
            i = i+1;
        }

        while(array->data[j] > pivotElement)
        {
            j = j-1;
        }

        if(i <= j)
        {

            int tmp = array->data[i];
            array_insert(array, array->data[j],i);
            array_insert(array, tmp,j);
            j=j-1;
            i=i+1;
        }
    }

    if ( low < j)
    {
        //printf("calling rec with low %d and high %d\n", low,j);
        quicksort(low, j, array);
    }
    if(high > i)
    {
        quicksort(i,high, array);
    }
}

/**************************************
 * parallel sorting
 **************************************/

recursion_tree_node_t *
init_new_parallel_sort(array_t * array)
{

    stack_t *l_seq_work_stack = (stack_t *)malloc(sizeof(stack_t));

    recursion_tree_node_t *l_current_pos = 
        (recursion_tree_node_t *)malloc(sizeof(recursion_tree_node_t));
    l_current_pos->to_sort = array;
    l_current_pos->sort_from = 0;
    l_current_pos->sort_to = array->length;
    l_current_pos->global_pos = 0;

    l_current_pos->created_left_child = 0;
    l_current_pos->created_right_child = 0;
    l_current_pos->stored_seq_work = 0;
    l_current_pos->initialized_for_partitioning = 0;
    l_current_pos->initialized_pivot_sort = 0;

    l_current_pos->parent = NULL;

    l_current_pos->seq_work = l_seq_work_stack;

    return l_current_pos;

}

    void
compute_seq_work(stack_t *seq_work_stack, array_t *result_array) {

    seq_work_t work* = (seq_work_t*)stack_pop(seq_work_stack);
    while(work != NULL) {
        quicksort(work->sort_from, work->sort_to, work->to_sort);

        int i = work->sort_from;
        while(i< work->sort_to) {
            array_insert(result_array,        //into this array
                    array_get(work->to_sort, i),    //this element
                    work->global_pos + i);          //at this position
            i++;
        }

        work = seq_work_t work* = (seq_work_t*)stack_pop(seq_work_stack);
    }


}

    int
par_pivot_pick(struct recursion_tree_node *n, int id)
{
    int set_size = ceil(sqrt(n->to_sort->length));
    
    //store old position
    recursion_tree_node_t* old_sort_pos = current_pos;

    //wait that all threads have stored old sorting branch
    pthread_barrier_wait(&sync_barrier);
    //initializing array for pivot sets
    pthread_mutex_lock(&pivot_mutex);
    if(current_pos->initialized_pivot_sort == 0) {
            current_pos->initialized_pivot_sort = 1;
            random_set = array_alloc(set_size); 
            picked = 0;
    }
    pthread_mutex_unlock(&pivot_mutex);

    //save current position in tree to free later
    recursion_tree_node_t *l_current_pos = current_pos;
    //store result array
    array_t l_random_set = random_set;

    //fetching random elements from array in parallel (fetch and increment)
    int rand;
    pos = __sync_fetch_and_add(&picked, 1);
    while(pos < set_size) {
        rand = random(n->to_sort->length);
        array_insert(random_set, rand, pos);
        pos = __sync_fetch_and_add(&picked, 1);
    }


    //initializing parallel sorting of pivots
    pthread_mutex_lock(&pivot_mutex);
    if(current_pos->initialized_pivot_sort == 1) {

        current_pos = init_new_parallel_sort(random_set);
        current_pos->initialized_pivot_sort = 2;
    }
    stack_t *l_seq_work = current_pos->seq_work;

    pthread_mutex_unlock(&pivot_mutex);

    par_partition(id);

    compute_seq_work(l_seq_work, l_random_set);

    //ensure that all threads are finished in order to guarantee the pivot in
    //its place
    pthread_barrier_wait(&sync_barrier);
    int pivot = array_get(random_set, floor((double)set_size/2.0));

    if(id == 0) {
        free(l_current_pos);
        free(l_random_set);
        current_pos = old_sort_pos;
        random_set = NULL;
    }

    return pivot;
}


/*
 * recursive parallel partitioning
 */
int
par_partition(int id) {

    //where are we now?
    struct recursion_tree_node l_current_pos = current_pos;

    if((l_current_pos->sort_to - l_current_pos->sort_from) <= L1_CACH_FALLBACK) 
    {
        if(!l_current_pos->stored_seq_work 
                && pthread_mutex_trylock(&work_adder_mutex)==0)
        {
            l_current_pos->stored_seq_work = 1;
            seq_work_t* new_elem =
                (seq_work_t*)malloc(sizeof(seq_work_t));
            stack_push(l_current_pos->seq_work, new_elem);
            new_elem->to_sort = l_current_pos->to_sort;
            new_elem->global_start_pos = l_current_pos->global_pos
            new_elem->sort_from = l_current_pos->sort_from;
            new_elem->sort_to = l_current_pos->sort_to;

            //wait till every thread checked local node before free
            pthread_barrier_wait(&sync_barrier);
            free(l_current_pos);

            pthread_mutex_unlock(&work_adder_mutex);

            return 1;

        } else
        {
            //wait for all others due to free
            pthread_barrier_wait(&sync_barrier);
            return 1;
        }
    }


    //if we are not a leave-> partition

    //find pivot in parallel
    int pivot = par_compute_pivot();

    int element_count = l_current_pos->sort_to - l_current_pos->sort_from;
    //create temp array for size of array (only once for all)
    pthread_mutex_lock(&temp_array_mutex);
        if(!l_current_pos->initialized_for_partitioning)
        {
            l_current_pos->partitioned = 
                array_alloc(element_count);

            left = 0;
            right = l_current_pos->partitioned->capacity-1;

            l_current_pos->initialized_for_partitioning = 1;
        }
    pthread_mutex_unlock(&temp_array_mutex);

    //run threw own chunk (n/p * thread_id)
    int chunk_size = ceil((double)element_count / (double)NB_THREADS);
    int start_point = id*chunk_size;
    int end = ((start_point + chunk_size) <= element_count ? start_point + chunk_size : element_count);
    int i;
    while(i = start_point; i<end; i++)
    {
        if(array_get(l_current_pos->to_sort, i) < pivot) {
            array_insert(l_current_pos->partitioned,
                    __sync_fetch_and_add(&left, 1));
        } else
        {
            array_insert(l_current_pos->partitioned,
                    __sync_fetch_and_add(&right, -1));
        }
    }


    //TODO check border values??
    pthread_mutex_lock(&create_child_mutex);
        if(!l_current_pos->created_left_child)
        {
            l_current_pos->pivot_pos = l_current_pos->left;

            current_pos = (struct recursion_tree_node*)malloc(sizeof(struct recursion_tree_node));

            current_pos->to_sort = l_current_pos->partitioned;
            current_pos->sort_from = 0;
            current_pos->sort_to = l_current_pos->pivot_pos-1;

            current_pos->created_left_child = 0;
            current_pos->created_left_child = 0;
            current_pos->stored_seq_work = 0;
            current_pos->initialized_for_partitionind = 0;
            current_pos->initialized_pivot_sort = 0;

            current_pos->global_pos = l_current_pos->global_pos;
            current_pos->seq_work = l_current_pos->seq_work;

            l_current_pos->created_left_child = 1;

        }
    pthread_mutex_unlock(&create_child_mutex);

    int left_child_is_leave = par_sort(id);

    //create new array for second recursion half
    pthread_mutex_lock(&create_child_mutex);
        if(!l_current_pos->created_right_child)
        {
            l_current_pos->pivot_pos = l_current_pos->right;

            current_pos = 
                (recursion_tree_node_t*)malloc(sizeof(struct recursion_tree_node));

            current_pos->to_sort = l_current_pos->partitioned;
            current_pos->sort_from = l_current_pos->pivot_pos;
            current_pos->sort_to = l_current_pos->partitioned->length-1;

            current_pos->created_left_child = 0;
            current_pos->created_left_child = 0;
            current_pos->stored_seq_work = 0;
            current_pos->initialized_for_partitionind = 0;
            current_pos->initialized_pivot_sort = 0;

            current_pos->global_pos = 
                l_current_pos->global_pos + l_current_pos->pivot_pos;
            current_pos->seq_work = l_current_pos->seq_work;

            l_current_pos->created_right_child = 1;

        }
    pthread_mutex_unlock(&create_child_mutex);

    int right_child_is_leave = par_sort(id);

    if(id == 0) {
        //if we have a leave child, this is still needed and freed later
        if(!(right_child_is_leave || left_child_is_leave))
            free(l_current_pos->partitioned);

        free(l_current_pos);
    }

    //not a leave
    return 0;
}

/*
 * runnning thread
 */
    static void *
run_thread(void* buffer);
{
    struct par_qsrt_thread* data;
    data = (struct par_sqrt_thread *)buffer;

    recursion_tree_node_t tree_root = current_pos;

    par_sort(data->id);

    compute_seq_work(tree_root->seq_work);

    return NULL;

}

/*
 * global function to sort
 */
    int
sort(struct array *array)
{
#if NB_THREADS > 0
    pthread_attr_t pthread_attr;

    seq_work_stack = (stack_t *)malloc(sizeof(stack_t));

    current_pos  = init_new_parallel_sort(array);
    recursion_tree_node_t l_current_pos = current_pos;

    pthread_attr_init(&pthread_attr);
    pthread_attr_setdetachedstate(&thread_attr, PTHREAD_CREATE_JOINABLE);

    int i;
    for(i=0; i<NB_THREADS; i++) 
    {
        thread_data[i].id = i;
        pthread_create(&thread[i], &pthread_attr, &run_thread, &thread_data[i]);

    }

    //wait till threads are finished
    for(i = 0; i<NB_THREADS; i++)
    {
        pthread_join(&thread[i], NULL);
    }

    free(l_current_pos);


#else
    //maybe an optimal seq. version is needed here instead of running the parallel one pseudo sequential.
    quicksort(0, array->length-1, array);
#endif

    return 0;
}




//sequential fallback
//sort array
//store items at current position into final array
//does this make sense? might not be really parallel, but in fact it would be and would save n*log(n) copies...
