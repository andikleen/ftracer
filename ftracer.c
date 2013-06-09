/*
 * Copyright (c) Andi Kleen
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/* Simple Linux kernel style ftracer for user programs */

#define _GNU_SOURCE 1
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>
#include <signal.h>

#include "ftracer.h"

#define TLEN (TSIZE / sizeof(struct trace))

asm(
"	.globl __fentry__\n"
"__fentry__:\n"
/* save arguments. must match frame below. */
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
	uint64_t func;
	uint64_t arg1, arg2, arg3;
	uint64_t rsp;
};

struct frame {
	uint64_t r9;
	uint64_t r8;
	uint64_t rcx;
	uint64_t rdx;
	uint64_t rsi;
	uint64_t rdi;
	uint64_t rax;
	uint64_t func;
	uint64_t caller; /* untraced currently */
};

int ftracer_size = TLEN;
bool ftracer_enabled;
__thread struct trace ftracer_tbuf[TLEN];
int ftracer_tcur;

double ftracer_frequency = 1.0;

/* No floating point can be used in this function. */
__attribute__((used)) void ftracer(struct frame *fr)
{
	if (!ftracer_enabled)
		return;
	struct trace *t = &ftracer_tbuf[ftracer_tcur++];
	if (ftracer_tcur >= TLEN)
		ftracer_tcur = 0;
	t->tstamp = __builtin_ia32_rdtsc();
	t->func = fr->func;
	t->arg1 = fr->rdi;
	t->arg2 = fr->rsi;
	t->arg3 = fr->rdx;
	t->rsp = (uint64_t)fr;
}

bool ftrace_enable(void)
{
	bool old = ftracer_enabled;
	ftracer_enabled = true;
	return old;
}

bool ftrace_disable(void)
{
	bool old = ftracer_enabled;
	ftracer_enabled = false;
	return old;
}

static const char *resolve(char *buf, int buflen, uint64_t addr)
{
	Dl_info info;
	if (dladdr((void *)addr, &info) && info.dli_sname)
		return info.dli_sname;
	else
		snprintf(buf, buflen, "%lx" ,addr);
	return buf;
}

static unsigned dump_start(unsigned max, unsigned cur)
{
	bool wrapped = ftracer_tbuf[cur].tstamp != 0;

	if (!max)
	       	return wrapped ? cur : 0;

	if (max > TLEN)
		max = TLEN;
	int o = (int)cur - max;
	if (o < 0)
		o = wrapped ? TLEN + o : 0;
	return o;
}

#define MAXSTACK 64

void ftrace_dump(FILE *out, unsigned max)
{
	int cur = ftracer_tcur;
	unsigned i;
	uint64_t ts = 0, last = 0;
	int stackp = 0;
	uint64_t stack[MAXSTACK];
	bool oldstate = ftrace_disable();

	fprintf(out, "%9s %9s %-25s %s\n", "TIME", "TOFF", "FUNC", "ARGS");
	for (i = dump_start(max, cur); ; i = (i + 1) % TLEN) {
		struct trace *t = &ftracer_tbuf[i];
		if (t->tstamp == 0)
			break;
		if (!ts) {
			ts = t->tstamp;
			stack[stackp] = t->rsp;
			last = t->tstamp;
		}

		/* Stack heuristic for nesting. Assumes the stack pointer stays
		   constant inside a function. Can break with shrink wrapping etc.
		   Also stack switches will confuse it. */
		if (t->rsp < stack[stackp]) {
			if (stackp < MAXSTACK)
				stack[++stackp] = t->rsp;
		} else {
			while (stackp > 0 && t->rsp > stack[stackp])
				stackp--;
		}

		char buf[128];
		char func[128];
		snprintf(buf, sizeof buf, "%-*s%s",
				2*stackp, "",
				resolve(func, sizeof func, t->func));
		fprintf(out, "%9.2f %9.2f %-25s %lx %lx %lx\n",
		       (t->tstamp - ts) / ftracer_frequency,
		       (t->tstamp - last) / ftracer_frequency,
		       buf,
		       t->arg1, t->arg2, t->arg3);
		last = t->tstamp;

		if (i == cur - 1)
			break;
	}

	if (oldstate)
		ftrace_enable();
}

static int dump_at_exit;

static void call_ftrace_dump(void)
{
	ftrace_dump(stderr, dump_at_exit);
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

static void ftracer_envconf(void)
{
	char *v = getenv("FTRACER");
	if (v && sscanf(v, "%d", &dump_at_exit) == 1) {
		printf("enabled %d\n",dump_at_exit);
		if (dump_at_exit == 1)
			dump_at_exit = 0;
		ftrace_dump_at_exit(dump_at_exit);
		// xxx chain previous handler
		// handle more signals?
		signal(SIGABRT, (__sighandler_t)call_ftrace_dump);
		ftrace_enable();
	}
}

static void __attribute__((constructor)) init_ftracer(void)
{
	ftracer_envconf();

	FILE *f = fopen("/proc/cpuinfo", "r");
	if (!f)
		goto fallback;

	char *line = NULL;
	size_t linelen = 0;
	ftracer_frequency = 0;
	while (getline(&line, &linelen, f) > 0) {
		char unit[10];

		if (strncmp(line, "model name", sizeof("model name")-1))
			continue;
		if (sscanf(line + strcspn(line, "@") + 1, "%lf%10s", 
			   &ftracer_frequency, unit) == 2) {
			if (!strcasecmp(unit, "GHz"))
				;
			else if (!strcasecmp(unit, "Mhz"))
				ftracer_frequency *= 1000.0;
			else {
				printf("Cannot parse unit %s\n", unit);
				goto fallback;
			}
			break;
		}
	}     
	free(line);
	fclose(f);
	if (ftracer_frequency) {
		ftracer_frequency *= 1000;
		return;
	}
    
fallback:
	f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", "r");
	int found = 0;
	if (f) {
		found = fscanf(f, "%lf", &ftracer_frequency);
		fclose(f);
	}
	if (found == 1) {
		ftracer_frequency /= 1000.0;
		return;
	}
	printf("Cannot find ftracer_frequency\n");
	ftracer_frequency = 1;
}

#if 0 /* Gives ugly warnings */
asm(" .pushsection \".debug_gdb_scripts\",\"MS\",@progbits,1\n"
    " .byte 1\n"
    " .asciz \"ftracer-gdb.py\"\n"
    " .popsection");
#endif

