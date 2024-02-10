#include <stdint.h>
#include <stdio.h>

typedef uint64_t u64;

#include "haversine_platform_metrics.cpp"

int main()
{
	printf("OS timer frequency: %llu\n", GetOSTimerFreq());	
	printf("CPU timer frequency: %llu\n", GuessCPUTimerFreq());
	return 0;
}