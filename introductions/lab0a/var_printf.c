/*
 * var_printf.c
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
#include <math.h>
#include <string.h>
#include <limits.h>

#define DEFINED_STRING "Defined string"
#define DYNAMIC_STRING "Dynamic string"

int
main(int argc, char ** argv)
{
	int integer, integer_overflow;
	char character;
	long long int long_long_int;
	float floating_1, floating_2;
	double double_precision;
	unsigned int unsigned_integer;
	int * int_pointer;
	char * dynamic_string;
	int static_integer_array[2];
	int * dynamic_integer_array;

	// Variable types and admissible values
	integer = 356;
	integer_overflow = INT_MAX;
	integer_overflow = 4 * integer_overflow; // More than what int can handle
	character = 'b';
	long_long_int = INT_MAX;
	long_long_int = long_long_int * 4;
	floating_1 = 5.2;
	floating_2 = sqrt(2);
	double_precision = sqrt(2);
	unsigned_integer = UINT_MAX;
	int_pointer = &integer;
	static_integer_array[0] = 10;
	static_integer_array[1] = 20;

	// Allocate dynamic integer array with 2 elements
	dynamic_integer_array = malloc(sizeof(int) * 2);
	dynamic_integer_array[0] = 30;
	dynamic_integer_array[1] = 40;

	// Dynamic string: array to character, allocated
	dynamic_string = malloc(sizeof(DYNAMIC_STRING) + 1);
	memcpy(dynamic_string, DYNAMIC_STRING, sizeof(DYNAMIC_STRING));

	// How to display these variables?
	printf("Integer (%dB): %d\n", sizeof(int), integer);
	printf("Integer (%dB, overflow): %d\n", sizeof(int), integer_overflow);
	printf("Character (%dB): %c\n", sizeof(char), character);
	printf("Character (as integer, %dB): %d\n", sizeof(char), character);
	printf("Long long integer (%dB): %Li\n", sizeof(long long int), long_long_int);
	printf("Float (%dB): %f\n", sizeof(float), floating_1);
	printf("Float (sqrt(2), %dB)): %.9f\n", sizeof(float), floating_2);
	printf("Double (sqrt(2), %dB): %.9f\n", sizeof(double), double_precision);
	printf("Unsigned int (%dB): %u\n", sizeof(unsigned int), unsigned_integer);
	printf("Unsigned int (as signed int, %dB): %d\n", sizeof(unsigned int), unsigned_integer);
	printf("Integer pointer (%dB): %X\n", sizeof(int*), int_pointer);
	printf("Integer pointer (pointed value, %dB): %d\n", sizeof(int), *int_pointer);
	printf("Static integer array (%dB): %X\n", sizeof(int*), static_integer_array);
	printf("Static integer array elements (2 * %dB): %d %d\n", sizeof(int), static_integer_array[0], static_integer_array[1]);
	printf("Dynamic integer array (%dB): %X\n", sizeof(int*), dynamic_integer_array);
	printf("Dynamic integer array elements (dynamic size): %d %d\n", dynamic_integer_array[0], dynamic_integer_array[1]);
	printf("Static string (%dB): %s\n", sizeof("Static string"), "Static string");
	printf("Defined string (%dB): %s\n", sizeof(DEFINED_STRING), DEFINED_STRING);
	printf("Dynamic string (%dB): %X\n", sizeof(char*), dynamic_string);
	printf("Dynamic string (dynamic size): %s\n", dynamic_string);
	printf("Argument count (integer, %dB): %d\n", sizeof(int), argc);
	printf("Argument 0 and 1 (dynamic string, dynamic size): %s, %s\n", argv[0], argv[1]);

	// Let's write beyond the dynamic integer array
	dynamic_integer_array[4] = 36;
	printf("Dynamic integer array 4th elements: %X\n", &dynamic_integer_array[4]);
	printf("Dynamic string: %s\n", dynamic_string);

	// Never forget to free dynamically allocated memory, or get memory leakage
	free(dynamic_integer_array);
	free(dynamic_string);

	return EXIT_SUCCESS;
}
