CFLAGS := -g

test: ftracer.o 

test.o: CFLAGS+=-pg -mfentry

clean:
	rm -f test.o test ftracer.o
