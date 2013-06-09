#ifndef FTRACER_H
#define FTRACER_H 1

void ftrace_enable(void);
void ftrace_disable(void);
void ftrace_dump(unsigned max);
void ftrace_dump_at_exit(unsigned max);

#endif
