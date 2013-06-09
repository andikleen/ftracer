CFLAGS := -g -Wall -O2 
# ftracer.o itself should not use those
# -fno-inline is optional (but inlined functions are not visible
# to the tracer)
TRACE_CFLAGS := -pg -mfentry -fno-inline
# Linker Flags needed for any traced program
# -lpthread is only needed when the gdb module is needed
# (gdb cannot access thread local data without it)
LDFLAGS := -ldl -rdynamic -lpthread
#
# Change for different trace buffer size per thread
# This is in bytes
CONFIG := -DTSIZE=128*1024

CLEAN := test.o bench.o ftracer.o test bench test.out test2.out \
	 test-thr.o
EXE := test bench test-thr

all: ftracer.o ${EXE}

test: ftracer.o 
bench: ftracer.o
test-thr: ftracer.o

ftracer.o: CFLAGS += ${CONFIG}

test.o: CFLAGS+=${TRACE_CFLAGS}
bench.o: CFLAGS+=${TRACE_CFLAGS}
test-thr.o: CFLAGS+=${TRACE_CFLAGS}

check: test
	./test 2>&1 | ./strip-log > test.out
	diff -b -u test.out test.out-expected

	gdb -x gdbtest --eval detach --eval quit ./test 2>&1 | \
		sed -n '1,/^Detaching/p' | \
		sed -n '/TIME/,$$p' | \
		grep -v Detaching | \
		./strip-log > test2.out
	diff -b -u test2.out test.out-expected
	echo PASSED
clean:
	rm -f ${CLEAN} ${EXE}
