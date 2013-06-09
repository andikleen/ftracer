#include "ftracer.h"
#include <pthread.h>
#include <unistd.h>

#define mb() asm volatile ("" ::: "memory")

volatile int go;

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

void *thread(void *arg)
{
	if (go == 0)
		mb();
	int i;
	for (i = 0; i < 5; i++) { 
		f1(1, 2, 3);
		f1(3, 4, 5);
	}
	sleep(5);
	return NULL;
}

#define NTHREADS 2

int main(void)
{
	ftrace_enable();
	int i;
	pthread_t thr[NTHREADS];
	for (i = 0; i < NTHREADS; i++) 
		pthread_create(&thr[i], NULL, thread, NULL);
	mb();
	go = 1;
	mb();
	thread(NULL);
	for (i = 0; i < NTHREADS; i++)
		pthread_join(thr[i], NULL);
	return 0;
}
