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

  return res;
}


int
stack_push(stack_t *stack, void* buffer)
{
  stack_elem_t* new_elem = (stack_elem_t *) malloc (sizeof(stack_elem_t));
  if(new_elem == NULL)
      return 1;

  new_elem->value = buffer;
  
stack_elem_t* old;
  do
  {
  	old = stack->head;
 	new_elem->next = old;
  // Hardware CAS-based stack
  }while(cas(&(stack->head), old, new_elem) != old ); 

  return 0;
}

void *
stack_pop(stack_t *stack)
{
	stack_elem_t* old, *new_head;
	do{
		old = stack->head;
		if(old == NULL)	//no elements on the stack
			break;
		else		//one or more elements on the stack
			new_head = old->next;
	//Hardware CAS-stack based stack
	}while(cas(&(stack->head),old,new_head) != old);
    void *ret = old->value;
	free(old);

  return ret;
}

