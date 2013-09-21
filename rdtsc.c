#include <stdio.h>

static inline uint64_t rdtsc(void)
{
    uint32_t eax, edx;
    __asm volatile ("rdtsc" : "=a" (eax), "=d" (edx));
    return ((uint64_t)edx << 32) | eax;
}

int main () {
  printf("rdtsc: %lu\n", rdtsc());
  return 0;
}
