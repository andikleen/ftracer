CFLAGS := -g -Wall -O2 
LDFLAGS := -ldl -rdynamic -lpthread

all: test ftracer.o bench

test: ftracer.o 

test.o: CFLAGS+=-pg -mfentry -fno-inline
bench.o: CFLAGS+=-pg -mfentry -fno-inline

clean:
	rm -f test.o test ftracer.o
