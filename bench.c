#include <stdint.h>
#include <stdio.h>
#define N 1000

#ifdef RDPMC
#include "rdpmc.h"
#define MEASURE() rdpmc_read(&cycles)
#else
#define MEASURE() __builtin_ia32_rdtsc()
#endif

#define mb() asm volatile("":::"memory")

__attribute__((no_instrument_function)) void f1(void)
{
	mb();
}

void f2(void)
{
	mb();
}

int main()
{
#ifdef RDPMC
	struct rdpmc_ctx cycles;
	rdpmc_open(0, &cycles);
#endif
	int j;
	for (j = 0; j < 10; j++) {
	f2();
	f1();

	uint64_t start = MEASURE();
	int i;
	for (i = 0; i < N; i++)
		f1();
	uint64_t end = MEASURE();

	printf("%d uninstrumented calls avg %lu cycles\n", N, (end-start)/N);

	start = MEASURE();
	for (i = 0; i < N; i++)
		f2();
	end = MEASURE();

	printf("%d instrumented calls avg %lu cycles\n", N, (end-start)/N);
	}
	return 0;
}
