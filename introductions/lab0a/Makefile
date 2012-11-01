##
## Makefile
##
##  Created on: 5 Sep 2011
##  Copyright 2011 Nicolas Melot
##
## This file is part of TDDD56.
## 
##     TDDD56 is free software: you can redistribute it and/or modify
##     it under the terms of the GNU General Public License as published by
##     the Free Software Foundation, either version 3 of the License, or
##     (at your option) any later version.
## 
##     TDDD56 is distributed in the hope that it will be useful,
##     but WITHOUT ANY WARRANTY; without even the implied warranty of
##     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##     GNU General Public License for more details.
## 
##     You should have received a copy of the GNU General Public License
##     along with TDDD56. If not, see <http://www.gnu.org/licenses/>.
## 
##

FILES=hello_world.c var_printf.c function_header.c module.c pthread.c Makefile
ARCHIVE=Lab0a.zip

CC=gcc
CFLAGS=-g -O0 -Wall

all: hello_world var_printf function_header pthread

clean:
	$(RM) hello_world
	$(RM) var_printf
	$(RM) pthread
	$(RM) function_header
	$(RM) *.o
	$(RM) *.zip

hello_world: hello_world.c
	$(CC) $(CFLAGS) -o hello_world hello_world.c

var_printf: var_printf.c
	$(CC) $(CFLAGS) -o var_printf var_printf.c

function_header: function_header.c module.h module.c
	$(CC) $(CFLAGS) -c -o module.o module.c
	$(CC) $(CFLAGS) -c -o function_header.o function_header.c
	$(CC) -o function_header module.o function_header.o

pthread: pthread.c
	$(CC) $(CFLAGS) -pthread -lm -o pthread pthread.c
	
.PHONY: all clean dist

dist:
	zip $(ARCHIVE) $(FILES)
