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

#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>

#include "array.h"
#include "sort.h"
#include "simple_quicksort.h"

#define L1_CACHE_THREASHOLD 10000
#define INSERTION_SORT_THREASHOLD 10

typedef struct array array_t;

#if NB_THREADS > 0
typedef struct thread_data {

    int id;
    array_t *to_sort;

} thread_data_t;

pthread_t thread[NB_THREADS];
thread_data_t thread_data[NB_THREADS];

array_t *global_array;
array_t *splitters;
array_t *splitter_candidates;

array_t *partitioning_lists[NB_THREADS];

int n_splitters;
int picked_splitters;
#endif

/*
 * sequential code
 */

int binarySearch(struct array* array, int value){
    int left = 0;
    int right = array->length - 1;
    int median = (array->length - 1) / 2;
    while(left < right - 1){
        if(value <= array_get(array, median)){
            right = median;
            median = median - (right - left) / 2 ;
        }else{
            left = median;
            median = median + (right - left) / 2 ;
        }   
    }
    if(value <= array_get(array,left))
        return left;
    else if(value > array_get(array, right))
        return right + 1;
    else
        return right;

}

    void
insertionsort(int low, int high, array_t *array)
{
    int i, j;
    int c;
    for(i = low + 1; i<=high; i++)
    {

        c = array->data[i];
        j = i;
        while(j > low && array->data[j - 1] > c) 
        {
            array_insert(array,
                    array->data[j-1],
                    j);
            j--;
        }

        array_insert(array, c, j);
    }
}


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
    mergeSort(low, middle,helperArray,array);
    mergeSort(middle+1,high,helperArray,array);
    merge(low, middle, high, helperArray, array);
  }
}



    void 
quicksort(int low, int high, array_t * array)
{
    if(array->length < INSERTION_SORT_THREASHOLD)
    {
        insertionsort(low, high, array);
        return;
    }

    // choose a good pivot element
    int i = 0;
    int pivotElement = 0;
    int pivotChoice = (int) sqrt(high-low);
    struct array *pivotElements  = array_alloc(pivotChoice); 
    for ( i = 0; i < pivotChoice; i++)
    {
        array_put(pivotElements,array->data[low+(random()%(high - low))]);
    }
    //this works since we fall back to insertionsort for low element numbers
    quicksort(0, pivotChoice -1, pivotElements);
    // choose median
    pivotElement = pivotElements->data[(int)(pivotChoice/2)];

    array_free(pivotElements);

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
        quicksort(low, j, array);
    }
    if(high > i)
    {
        quicksort(i,high, array);
    }
    
}

#if NB_THREADS > 0
/*
 * parallel code
 */


   void 
pick_splitters()
{

	int nb_splitter_candidates = sqrt(global_array->length);
	int priv_pick_count;
	int pos;
	for(priv_pick_count=0; priv_pick_count < nb_splitter_candidates; priv_pick_count++)
	{
	        //compute random position in the range
		pos = random() % global_array->length;

	        //insert element from that position into splitter array
		array_insert(splitter_candidates,
			array_get(global_array, pos),
			priv_pick_count);

	}

	quicksort(0, nb_splitter_candidates -1, splitter_candidates);

	int jump = nb_splitter_candidates / NB_THREADS;
	pos = jump;
	int i;
	for(i=0; i<NB_THREADS - 1; i++) {
		array_put(splitters,
			array_get(splitter_candidates, pos)
		);
		
		pos = pos + jump;
	}
/*
int i = 0;

for(i=0;i<n_splitters;i++)
    array_put(splitters,
	array_get(global_array,random()%global_array->length));
for(i=1;i<=n_splitters;i++)
    array_put(splitters,
	array_get(global_array,global_array->length/i));
    return 0;
*/
}

	static void*
run_thread(void* buffer) 
{

	thread_data_t *data = (thread_data_t *)buffer;
	array_t *to_sort = data->to_sort;
	int i;
	array_t *helper_array = array_alloc(to_sort->length);
	for(i=0;i<to_sort->length;i++)
	{
		array_put(helper_array, array_get(to_sort, i));
	}

	mergeSort(0, to_sort->length - 1, helper_array, to_sort);

	return NULL;
}

#endif

	int
sort(struct array * array)
{

#if NB_THREADS <= 0
	array_t *helper_array = array_alloc(array->length);
	int i;
	for(i=0;i<array->length;i++)
	{
		array_put(helper_array, array_get(array, i));
	}
	mergeSort(0, array->length -1, helper_array, array);

#else
	global_array = array;
	n_splitters = NB_THREADS -1;

	if(n_splitters <= 0) 
	{
		array_t *helper_array = array_alloc(array->length);
		int i;
		for(i=0;i<array->length;i++)
		{
			array_put(helper_array, array_get(array, i));
		}
		mergeSort(0, array->length - 1, helper_array, array);
	}
	else
	{
		picked_splitters = 0;

		splitters = array_alloc(n_splitters);
		splitter_candidates = array_alloc(floor(sqrt(array->length)));

		int i, j;
		for(i = 0; i< NB_THREADS; i++) 
		{
			//allocate worst case space which is size of all elements
			//partitioned by one thread in every list
			partitioning_lists[i] =  
				array_alloc(array->length);
		}



		pick_splitters();

		int end_pos = global_array->length;
		//put own share into corresponding splitter stacks
		for(i= 0; i < end_pos; i++) 
		{
			j=binarySearch(splitters, global_array->data[i]);

			array_put(partitioning_lists[j], global_array->data[i]);
		}


		//init threads
		pthread_attr_t pthread_attr;
		pthread_attr_init(&pthread_attr);
		pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_JOINABLE);

		//seed for random splitter and pivot picks
		srandom(array->data[0]);

		for(i = 0; i < NB_THREADS; i++) 
		{
			thread_data[i].id = i;
			thread_data[i].to_sort = partitioning_lists[i];
			pthread_create(&thread[i], &pthread_attr, &run_thread, &thread_data[i]);
		}

		int pos = 0;
		for(i= 0; i < NB_THREADS; i++)
		{
			pthread_join(thread[i], NULL);
			int j;
				
			for(j=0; j<partitioning_lists[i]->length; j++)
				array_insert(array,
					partitioning_lists[i]->data[j],
					pos + j);
			
			pos+=partitioning_lists[i]->length;
					
		}

		//Frees
		array_free(splitters);
		array_free(splitter_candidates);
		for(i=0;i<NB_THREADS;i++)
		{
			array_free(partitioning_lists[i]);
		}

	}
#endif

	return 0;
}


