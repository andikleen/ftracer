#include <stdint.h>
#include <stdio.h>
#define N 1000

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
	int j;
	for (j = 0; j < 10; j++) {
	f2();
	f1();

	uint64_t start = __builtin_ia32_rdtsc();
	int i;
	for (i = 0; i < N; i++)
		f1();
	uint64_t end = __builtin_ia32_rdtsc();

	printf("%d uninstrumented calls avg %lu cycles\n", N, (end-start)/N);

	start = __builtin_ia32_rdtsc();
	for (i = 0; i < N; i++)
		f2();
	end = __builtin_ia32_rdtsc();

	printf("%d instrumented calls avg %lu cycles\n", N, (end-start)/N);
	}
	return 0;
}
