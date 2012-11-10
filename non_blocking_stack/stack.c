/*
 * stack.c
 *
 *  Created on: 18 Oct 2011
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
 *     but WITHOUT ANY WARRANTY without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 * 
 *     You should have received a copy of the GNU General Public License
 *     along with TDDD56. If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#ifndef DEBUG
#define NDEBUG
#endif

#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "stack.h"
#include "non_blocking.h"

#if NON_BLOCKING == 0
#warning Stacks are synchronized through locks
#else
#if NON_BLOCKING == 1 
#warning Stacks are synchronized through lock-based CAS
#else
#warning Stacks are synchronized through hardware CAS
#endif
#endif

stack_t *
stack_alloc()
{
  // Example of a task allocation with correctness control
  // Feel free to change it
  stack_t *res;

  res = malloc(sizeof(stack_t));
  assert(res != NULL);

  if (res == NULL)
    return NULL;

// You may allocate a lock-based or CAS based stack in
// different manners if you need so
#if NON_BLOCKING == 0
  // Implement a lock_based stack
#else
  // Implement a CAS-based stack
#endif

  return res;
}

int
stack_init(stack_t *stack, size_t size)
{
  assert(stack != NULL);
  assert(size > 0);

#if NON_BLOCKING == 0
  // Implement a lock_based stack
  pthread_mutex_init(&(stack->mutex),NULL);
#else
  // Implement a CAS-based stack
#endif

  return 0;
}

int
stack_check(stack_t *stack)
{
  assert(stack != NULL);

  return 0;
}

int
stack_push_safe(stack_t *stack, void* buffer)
{


  stack_elem_t* new_elem = (stack_elem_t *) malloc (sizeof(stack_elem_t));
  new_elem->value = (int *)buffer;
  
#if NON_BLOCKING == 0
  // Implement a lock_based stack
  
  pthread_mutex_lock(&(stack->mutex));
  if(stack->head == NULL){
  		stack->head = new_elem;
  		stack->head->next = NULL;
  }else{
  		new_elem->next = stack->head;
  		stack->head = new_elem;
  }
  	pthread_mutex_unlock(&(stack->mutex));
  
#else
#if NON_BLOCKING == 1
	//Software CAS-based stack
#else 
  // Hardware CAS-based stack
  stack_elem_t* old;
  do
  {
  	old = stack->head;
 	new_elem->next = old;
  }while(!cas(stack->head, old, new_elem)); 
#endif
#endif

  return 0;
}

int
stack_pop_safe(stack_t *stack, void* buffer)
{
#if NON_BLOCKING == 0
  // Implement a lock_based stack
  pthread_mutex_lock(&(stack->mutex));
  if(stack->head == NULL)		//no elements on the stack
  		;
  else{	//one ore more elements on the stack
  		stack_elem_t* temp = stack->head;
  		stack->head = stack->head->next;
  		free(temp);
  }
  pthread_mutex_unlock(&(stack->mutex));
#else
#if NON_BLOCKING == 1
  	//Software CAS-based stack
#else
	//Hardware CAS-stack based stack
	stack_elem_t* old, *new_head;
	do{
		old = stack->head;
		if(old == NULL)	//no elements on the stack
			break;
		else{					//one or more elements on the stack
			new_head = old->next;
		}
	}while(!cas(stack->head,old,new_head));
	free(old);
#endif
#endif

  return 0;
}

