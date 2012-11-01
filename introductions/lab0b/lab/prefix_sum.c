#include <stdio.h>
#include <stdlib.h>

#include "array.h"

int
prefix_sum(struct array * array)
{
	return 0;
}

int
main(int argc, char** argv)
{
  // Declarations
  struct array * array;

  // Load input data from file
  array = array_read(argv[1]);

  // Compute and output result
  printf("Computing summation... %i\n", prefix_sum(array));

  // Free memory out
  array_free(array);

  // Everything went well
  return 0;
}
