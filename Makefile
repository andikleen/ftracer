CFLAGS := -g -Wall -O2 
LDFLAGS := -ldl -rdynamic -lpthread

test: ftracer.o 

test.o: CFLAGS+=-pg -mfentry -fno-inline

clean:
	rm -f test.o test ftracer.o
