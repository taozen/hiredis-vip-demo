CC=gcc -Wall -g -O2

test-proc: test.c
	$(CC) test.c -o test-proc -L/usr/local/lib -lhiredis_vip

test: test-proc
	./test-proc

clean:
	rm -f *.o test-proc

.PHONY: clean

