#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "array.h"

int
prefix_sum(struct array * array)
{
	return 0;
}

int
main(int argc, char** argv)
{
  struct array * array;
  int size, min, max, i;

  size = atoi(argv[1]);
  min = atoi(argv[2]);
  max = atoi(argv[3]);
  array = array_alloc(size);

  srandom(time(NULL));

  for(i = 0; i < size; i++)
  {
    array->data[i] = min + (random() % (max - min));
  }

  array_write(array, argv[4]);
  array_free(array);

  return 0;
}
