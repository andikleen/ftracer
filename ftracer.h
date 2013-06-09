#ifndef FTRACER_H
#define FTRACER_H 1

#include <stdbool.h>
#include <stdio.h>

bool ftrace_enable(void);
bool ftrace_disable(void);
void ftrace_dump(FILE *fh, unsigned max);
void ftrace_dump_at_exit(unsigned max);

#endif
