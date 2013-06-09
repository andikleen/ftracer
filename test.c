#include "ftracer.h"

#define mb() asm volatile ("" ::: "memory")

void f3(int a, int b, int c)
{
	mb();
}

void f2(int a, int b, int c)
{
	f3(1, 2, 3);
	f3(4, 5, 6);
}

void f1(int a, int b, int c)
{
	f2(10, 11, 12);
	f2(20, 21, 22);
	f3(7, 8, 9);
}

int main(void)
{
	ftrace_dump_at_exit(0);
	ftrace_enable();
	f1(1, 2, 3);
	f1(3, 4, 5);
	return 0;
}
