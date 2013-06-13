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
	 test-thr.o bench-rdpmc.o bench-rdpmc
EXE := test bench test-thr

all: ftracer.o ${EXE}

test: ftracer.o 
bench: ftracer.o
test-thr: ftracer.o

ftracer.o: CFLAGS += ${CONFIG}

test.o: CFLAGS+=${TRACE_CFLAGS}
bench.o: CFLAGS+=${TRACE_CFLAGS}
test-thr.o: CFLAGS+=${TRACE_CFLAGS}

# needs git://github.com/andikleen/pmu-tools
PMUTOOLS := ~/pmu/pmu-tools

bench-rdpmc.o: bench.c
	${CC} ${CFLAGS} -o bench-rdpmc.o -c $<

bench-rdpmc.o: CFLAGS += ${TRACE_CFLAGS} -DRDPMC -I ${PMUTOOLS}/self

bench-rdpmc: ftracer.o ${PMUTOOLS}/self/rdpmc.o

check: test test-thr
	./test 2>&1 | ./strip-log > test.out
	diff -b -u test.out test.out-expected

	# single threaded gdb
	gdb -x gdbtest --eval quit ./test 2>&1 | \
		sed -n '1,/^Detaching/p' | \
		sed -n '/TIME/,$$p' | \
		grep -v Detaching | \
		./strip-log > test2.out
	diff -b -u test2.out test.out-expected

	# threaded gdb check
	# output not checked for now
	gdb -x gdbtest-thr --eval quit ./test-thr

clean:
	rm -f ${CLEAN} ${EXE}
