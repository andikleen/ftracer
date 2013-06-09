/* Simple Linux kernel style ftracer for user programs */
#define _GNU_SOURCE 1
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>

#include "ftracer.h"

asm(
"	.globl __fentry__\n"
"__fentry__:\n"
/* save arguments */
"	push %rax\n"
"	push %rdi\n"
"	push %rsi\n"
"	push %rdx\n"
"	push %rcx\n"
"	push %r8\n"
"	push %r9\n"
"	movq %rsp,%rdi\n"
"	call ftracer\n"
"	pop %r9\n"
"	pop %r8\n"
"	pop %rcx\n"
"	pop %rdx\n"
"	pop %rsi\n"
"	pop %rdi\n"
"	pop %rax\n"
"	ret\n");

struct trace {
	uint64_t tstamp;
	uint64_t src;
	uint64_t dst;
	uint64_t arg1, arg2, arg3;
};

struct frame {
	uint64_t r9;
	uint64_t r8;
	uint64_t rcx;
	uint64_t rdx;
	uint64_t rsi;
	uint64_t rdi;
	uint64_t rax;
	uint64_t callee;
	uint64_t caller;
};

#define TSIZE 32768

static volatile bool tenabled;
static __thread struct trace tbuf[TSIZE];
static int tcur;

static double frequency;

__attribute__((used)) void ftracer(struct frame *fr)
{
	if (!tenabled)
		return;
	struct trace *t = &tbuf[tcur++];
	if (tcur >= TSIZE)
		tcur = 0;
	t->tstamp = __builtin_ia32_rdtsc();
	t->src = fr->caller;
	t->dst = fr->callee;
	t->arg1 = fr->rdi;
	t->arg2 = fr->rsi;
	t->arg3 = fr->rdx;
}

bool ftrace_enable(void)
{
	bool old = tenabled;
	tenabled = true;
	return old;
}

bool ftrace_disable(void)
{
	bool old = tenabled;
	tenabled = false;
	return old;
}

static const char *resolve(char *buf, int buflen, uint64_t addr)
{
	Dl_info info;
	if (dladdr((void *)addr, &info))
		return info.dli_sname;
	snprintf(buf, buflen, "%lx" ,addr);
	return buf;
}

static unsigned dump_start(unsigned max, unsigned cur)
{
	if (!max)
		max = TSIZE;
	if (tbuf[cur].tstamp) { /* Did it wrap? */
		if (max >= TSIZE)
			return (cur + 1) % TSIZE;
		if (max > cur)
			return (TSIZE - 1 - (max - (cur - max))); // XXX
	} else { 
		if (max >= cur)
			return 0;
	}
	return cur - max;
}

void ftrace_dump(unsigned max)
{
	struct trace *t;
	int cur = tcur;
	int i;
	uint64_t ts = 0, last = 0;
	bool oldstate = ftrace_disable();

	for (i = dump_start(max, cur); i != cur; i = (i + 1) % TSIZE) {
		t = &tbuf[i];
		if (t->tstamp == 0)
			break;
		if (!ts)
			ts = t->tstamp;
		if (!last)
			last = t->tstamp;
		char src[128], dst[128];
		printf("%6.1f %6.1f %-20s -> %-20s %lx %lx %lx\n",
		       (t->tstamp - ts) / frequency,
		       (t->tstamp - last) / frequency,
		       resolve(src, sizeof src, t->src),
		       resolve(dst, sizeof dst, t->dst),
		       t->arg1, t->arg2, t->arg3);
		last = t->tstamp;
	}

	if (oldstate)
		ftrace_enable();
}

static int dump_at_exit;

static void call_ftrace_dump(void)
{
	ftrace_dump(dump_at_exit);
}

void ftrace_dump_at_exit(unsigned max)
{
	static bool setup = false;
	if (setup)
		return;
	setup = true;
	dump_at_exit = max;
	atexit(call_ftrace_dump);
}

static void __attribute__((constructor)) init_ftracer(void)
{
     FILE *f = fopen("/proc/cpuinfo", "r");
     if (!f)
	  goto fallback;

     char *line = NULL;
     size_t linelen = 0;
     frequency = 0;
     while (getline(&line, &linelen, f) > 0) {
	  char unit[10];

	  if (strncmp(line, "model name", sizeof("model name")-1))
	       continue;
	  if (sscanf(line + strcspn(line, "@") + 1, "%lf%10s", 
		     &frequency, unit) == 2) {
	       if (!strcasecmp(unit, "GHz"))
		     ;
	       else if (!strcasecmp(unit, "Mhz"))
		     frequency *= 1000.0;
	       else {
		    printf("Cannot parse unit %s\n", unit);
		    goto fallback;
	       }
	       break;
	  }
     }     
     free(line);
     fclose(f);
     if (frequency)
	  return;
    
fallback:
     f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", "r");
     int found = 0;
     if (f) {
         found = fscanf(f, "%lf", &frequency);
	 fclose(f);
     }
     if (found == 1) {
         frequency /= 1000000.0;
         return;
     }
     printf("Cannot find frequency\n");
     frequency = 1;
}
