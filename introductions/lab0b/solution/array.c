/*
 * array.c
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
#include <assert.h>

#include "array.h"

static int
read_value(FILE * file, value * buffer)
{
  assert(file != NULL);
  int num, total;
  char read, has_more;

  read = 0;
  has_more = 1;
  num = 0;
  total = 0;

  while (read != ' ')
    {
      if ((has_more = fread(&read, 1, 1, file)) == 0 || read == ' ')
        {
          break;
        }

      total += has_more;
      num = num * 10 + read - '0';
    }

  *buffer = num;
  return total;
}

struct array*
array_alloc(int size)
{
  struct array* array;

  array = malloc(sizeof(array));
  assert(array != NULL);

  array->length = size;

  array->data = malloc(sizeof(value) * array->length);
  assert(array->data != NULL);

  return array;
}

struct array*
array_read(char * filename)
{
  int i;
  FILE * h;
  int num;
  struct array * res;

  h = fopen(filename, "r");
  assert(read_value(h, &num) > 0);

  res = array_alloc(num);

  num = 1;
  for (i = 0; i < res->length && num > 0; i++)
    {
      num = read_value(h, &res->data[i]);
    }

  fclose(h);

  return res;
}

void
array_printf(struct array* array)
{
  int i;

  for (i = 0; i < array->length; i++)
    {
      printf("%d ", array->data[i]);
    }
  printf("\n");
}

int
array_write(struct array* array, char* filename)
{
  int i;
  FILE * h;

  h = fopen(filename, "w");

  fprintf(h, "%i ", array->length);
  for (i = 0; i < array->length; i++)
    {
      fprintf(h, "%i ", array->data[i]);
    }

  return -1;
}

int array_check_ascending(struct array* array)
{
  int i;

  for(i = 0; i < array->length - 1; i++)
    {
      assert(array->data[i] <= array->data[i + 1]);
    }

  return array->length;
}

void
array_free(struct array* array)
{
  free(array->data);
  free(array);
}

