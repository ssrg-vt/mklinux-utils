
#include "bits.h"

//http://graphics.stanford.edu/~seander/bithacks.html
#define CHAR_BIT 8
#define T long long
int bit_weight(long long v) {
  int c;

  v = v - ((v >> 1) & ~0UL/3);
  // r = (r & 0x3333...) + ((r >> 2) & 0x3333...);
  v = (v & ~0UL/5) + ((v >> 2) & ~0UL/5);
  // r = (r & 0x0f0f...) + ((r >> 4) & 0x0f0f...);
  v = (v + (v >> 4)) & ~0UL/17;
  // r = r % 255;
  c = (v * (~0UL/255)) >> ((sizeof(v) - 1) * CHAR_BIT);

  return c;
}

int bit_weight_bitmask (cpu_bitmask_t * ptr) {
  int c = 0, i;
  long long * aptr = (long long *) ptr;

  for (i = 0 ; i < (sizeof(cpu_bitmask_t) / sizeof(long long)); i++)
    c += bit_weight( *(aptr++) );

  return c;
}

int ffsll_bitmask( cpu_bitmask_t * ptr) {
  int c = 0, d =0, i;
  long long * aptr = (long long *) ptr;

  for (i = 0 ; i < (sizeof(cpu_bitmask_t) / sizeof(long long)); i++)
    if ( d = ffsll(*(aptr++)) )
      break;
    else
      c += (sizeof(long long)* CHAR_BIT);

  if (d == 0)
    return 0;
  else
    return (c + d);
}

void clearcpu_bitmask( cpu_bitmask_t * ptr, int cpu_num) {
  int c = 0, d =0;
  long long * aptr = (long long *) ptr;
  aptr = aptr + (cpu_num / (sizeof(long long) * CHAR_BIT)); // select right long long
  cpu_num %= (sizeof(long long) * CHAR_BIT); // select right cpu num
  *aptr &= (unsigned long long)~((unsigned long long)1<<cpu_num); //mask it!
}

void print_bitmask( cpu_bitmask_t * ptr) {
  int i;
  long long * aptr = (long long *) ptr;

  for (i = 0 ; i < (sizeof(cpu_bitmask_t) / sizeof(long long)); i++)
    printf("%016llx,", *(aptr + (sizeof(cpu_bitmask_t) / sizeof(long long)) -i -1));
}
