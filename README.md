ftracer

ftracer is a simple user space implementation of a linux kernel style function tracer.
It allows to trace every call in a instrumented user applications. It is useful
for debugging and performance analysis due to its fine grained time stamp.

It relies on gcc generating a call on top of every function call.

The tracing slows every function call down. The tracing is per thread and does
not create a global bottleneck.

Note that the time stamps include the overhead from the tracing.

Requires gcc 4.7+ and a x86_64 system and the ability to rebuild the program.

Quick howto:
     cd ftrace
     make
     ./test
     TIME      TOFF CALLER                       CALLEE               ARGUMENTS
     0.00      0.00 main + 41                 -> f1                   1 2 3
     0.07      0.07   f1 + 25                 -> f2                   4016d4 b c
     0.11      0.04     f2 + 25               -> f3                   1 2 3
     0.15      0.04   f1 + 25                 -> f3                   4 5 6
     0.20      0.05   f1 + 45                 -> f2                   4016d7 15 16
     0.22      0.02     f2 + 25               -> f3                   1 2 3
     0.25      0.03   f1 + 45                 -> f3                   4 5 6
     0.29      0.04 main + 41                 -> f3                   7 8 9
     0.33      0.04 main + 61                 -> f1                   3 4 5
     0.37      0.04   f1 + 25                 -> f2                   4016d4 b c
     0.40      0.03     f2 + 25               -> f3                   1 2 3
     0.43      0.03   f1 + 25                 -> f3                   4 5 6
     0.47      0.04   f1 + 45                 -> f2                   4016d7 15 16
     0.51      0.04     f2 + 25               -> f3                   1 2 3
     0.55      0.04   f1 + 45                 -> f3                   4 5 6
     0.59      0.04 main + 61                 -> f3                   7 8 9

Performance overhead

On a Westmere system the instrumentation increases the cost of an empty call by
about 4 times. This is with a micro benchmark that does these calls in a tight
loop. On codes doing less function calls the overhead will likely be lower,
as an Out-of-order CPU can better schedule around it.
Exact slowdowns will depend on the CPU and the surrounding code and how many
function calls it does.

To use in your own project. 

The program needs to be build with -pg -mfentry and linked with -rdynamic -ldl ftrace.o

	cd my-project
	make CFLAGS='-g -pg -mfentry' LDFLAGS='-rdynamic -ldl ../ftrace/ftrace.o'	(or CXXFLAGS if using C++)
	make sure to rebuild everything

	FTRACER_ON=1 ./my_program

FTRACE_ON=1 enables automatic dumping at exit or SIGABRT (e.g. assert failure)

Control ftracer from the program:

	#include "ftracer.h"

	ftrace_enable();
	...
	ftrace_disable();

	ftrace_dump(100);	/* Dump last 100 entries of current thread to stdout */

Call ftrace_dump_on_exit(max) to automatically dump on exit. ftrace_dump() can be also 
called from gdb during debugging.

A common use case is to keep the tracer running, but dump when
something odd happens (like an assertation failure)

The trace buffer size per thread is hard coded, but can be changed
in the Makefile and rebuilding ftracer.o. The default is 32k

Limits:

static symbols cannot be resolved to names right now (compile with -Dstatic= if needed)
The tracer cannot see uninstrumented and inlined functions.
There are some circumstances that confuse the nesting heuristic.
To trace dynamically linked functions in standard libraries you can use ltrace instead.

Thread interaction:

The thread buffer is per process and is thread safe. However
ftrace_dump will only dump the current process and the automatic exit
on dumping will only dump the thread that called exit (this may be
fixed in the future)

