/* Simple Linux kernel style ftracer for user programs */
#define _GNU_SOURCE 1
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

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

void ftrace_enable(void)
{
	tenabled = true;
}

void ftrace_disable(void)
{
	tenabled = false;
}

static char *resolve(char *buf, int buflen, uint64_t addr)
{
	Dl_info info;
	if (dladdr((void *)addr, &info))
		return info.dli_sname;
	snprintf(buf, buflen, "%lx" ,addr);
	return buf;
}

void ftrace_dump(unsigned max)
{
	int num = 0;
	struct trace *t;
	int cur = tcur;
	int i = cur - 1;

	do {
		if (i < 0)
			i = TSIZE-1;
		t = &tbuf[i];
		i--;
		if (t->tstamp == 0)
			break;

		char src[128], dst[128];
		printf("%lx %s->%s %lx %lx %lx\n",
				t->tstamp,
				resolve(src, sizeof src, t->src),
				resolve(dst, sizeof dst, t->dst),
				t->arg1, t->arg2, t->arg3);
		
		num++;
	} while (i != cur && !(max && num < max));
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
