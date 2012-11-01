/*
 * erray.h
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

#ifndef ERRAY_H_
#define ERRAY_H_

typedef int value;

struct array
{
  int length;
  value* data;
};

struct array * array_read(char *);
void array_printf(struct array*);
void array_(int *);
struct array * array_alloc(int);
void array_free(struct array*);
int array_write(struct array*, char*);
int array_check_ascending(struct array*);

#endif /* ERRAY_H_ */
