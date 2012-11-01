CFLAGS=-g -O0 -Wall
FILES=array.* Makefile prefix_sum.c compile merge plot_data.m run settings start time_*.m variables rand.c
ARCHIVE=Lab0b-solution.zip
NB_THREADS=0

all: prefix_sum rand

clean:
	$(RM) rand
	$(RM) prefix_sum
	$(RM) prefix_sum-*
	$(RM) *.o
	$(RM) *.zip

rand: array.o rand.c
	gcc $(CFLAGS) -o rand array.o rand.c
	
prefix_sum: prefix_sum.c array.o
	gcc $(CFLAGS) -o prefix_sum$(SUFFIX) array.o prefix_sum.c -lrt -DNB_THREADS=$(NB_THREADS)

array.o: array.c
	gcc $(CFLAGS) -c -o array.o array.c

.PHONY: all clean dist

dist:
	zip $(ARCHIVE) $(FILES)
