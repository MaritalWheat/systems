libmem.so: mem.o
	gcc -fpic -shared -o libmem.so mem.o

mem.o: mem.c mem.h
	gcc -c -fpic mem.c -Wall -Werror

clean: 
	rm -f *.o *.txt
