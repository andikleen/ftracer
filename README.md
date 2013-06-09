ftracer

ftracer is a simple user space implementation of a linux kernel style function tracer.
It allows to trace every call in a instrumented user applications. It is useful
for debugging and performance analysis due to its fine grained time stamp.

It relies on gcc generating a call on top of every function call.

The tracing slows every function call down. The tracing is per thread and does
not create a global bottleneck.

Note that the time stamps include the overhead from the tracing.

Requires gcc 4.7+ and a x86_64 Linux system and the ability to rebuild the program.

Quick howto:
     cd ftrace
     make
     ./test
     TIME      TOFF FUNC                      ARGS
     0.00      0.00 f1                        1 2 3
     0.06      0.06   f2                      a b c
     0.10      0.04     f3                    1 2 3
     0.14      0.04   f3                      4 5 6
     0.18      0.03   f2                      14 15 16
     0.20      0.03     f3                    1 2 3
     0.23      0.03   f3                      4 5 6
     0.27      0.04 f3                        7 8 9
     0.30      0.03 f1                        3 4 5
     0.33      0.03   f2                      a b c
     0.35      0.03     f3                    1 2 3
     0.39      0.04   f3                      4 5 6
     0.42      0.03   f2                      14 15 16
     0.45      0.03     f3                    1 2 3
     0.49      0.04   f3                      4 5 6
     0.53      0.04 f3                        7 8 9


Performance overhead

On a Westmere system the instrumentation increases the cost of an empty call by
about 6 times. This is with a micro benchmark that does these calls in a tight
loop. On codes doing less function calls the overhead will likely be lower,
as an Out-of-order CPU can better schedule around it.
Exact slowdowns will depend on the CPU and the surrounding code and how many
function calls it does.

To use in your own project. 

The program needs to be build with -pg -mfentry and linked with -rdynamic -ldl ftrace.o

	cd my-project
	make CFLAGS='-g -pg -mfentry' LDFLAGS='-rdynamic -ldl ../ftrace/ftrace.o'
	(or CXXFLAGS if using C++)
	make sure to rebuild everything

	FTRACER=1 ./my_program

FTRACER=1 enables automatic dumping at exit or SIGABRT (e.g. assert failure)
If a number other than 0 and 1 is specified it specifies the number of entries to dump.

Control ftracer from the program:

	#include "ftracer.h"

	ftrace_enable();
	...
	ftrace_disable();

	ftrace_dump(stdout, 100);	/* Dump last 100 entries of current thread to stdout */

Call ftrace_dump_on_exit(max) to automatically dump on exit. ftrace_dump() can be also 
called from gdb during debugging.

A common use case is to keep the tracer running, but dump when
something odd happens (like an assertation failure)

The thread buffer is per process and is thread safe. However
ftrace_dump will only dump the current process and the automatic exit
on dumping will only dump the thread that called exit.

The trace buffer can be also controlled from gdb using a special python module.
This has the advantage that gdb can display the trace buffers from all threads.
Note that the program needs to be linked with -lpthread to allow gdb to access
the per thread buffers

	FTRACER=1 gdb program
        ...
	(gdb) source ftracer-gdb.py	
	...
	<program stops e.g. at a break point or signal>
        (gdb) ftracer

	[Switching to thread 1 (Thread 0x7ffff7615700 (LWP 23177))]
	#0  main () at test.c:28
	28              f1(3, 4, 5);
        TIME    DELTA THR FUNC                      ARGS
        0.00     0.00   1 f1                        1 2 3
        0.03     0.03   1   f2                      a b c
        0.05     0.02   1     f3                    1 2 3
        ...

When multiple threads are active they are displayed interleaved side by side.
With many threads this may require a very wide terminal (plus a very small font)
This can be useful to look for race conditions. This mode is only supported
by gdb, not by the C dumper.

Limits:

The trace buffer size per thread is hard coded, but can be changed
in the Makefile and rebuilding ftracer.o.
static symbols cannot be resolved to names right now from the program
(compile with -Dstatic= if needed or use the gdb ftracer command)
The tracer cannot see uninstrumented and inlined functions.
There are some circumstances that confuse the nesting heuristic.
With gcc 4.8 you may need to also disable shrink-wrapping.
To trace dynamically linked functions in standard libraries -- like
malloc -- you can use ltrace instead.


