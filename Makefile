CFLAGS := -g -Wall -O2 
# ftracer.o itself should not use those
# -fno-inline is optional (but inlined functions are not visible
# to the tracer)
TRACING_CFLAGS := -pg -mfentry -fno-inline
# Linker Flags needed for any traced program
# -lpthread is only needed when the gdb module is needed
# (gdb cannot access thread local data without it)
LDFLAGS := -ldl -rdynamic -lpthread

CLEAN := test.o bench.o ftracer.o test bench
EXE := test bench

all: ftracer.o ${EXE}

test: ftracer.o 

test.o: CFLAGS+=${TRACE_CFLAGS}
bench.o: CFLAGS+=${TRACE_CFLAGS}

clean:
	rm -f ${CLEAN} ${EXE}
