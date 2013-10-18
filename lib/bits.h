
int bit_weight(long long v);

int bit_weight_bitmask (cpu_bitmask_t * ptr);

int ffsll_bitmask( cpu_bitmask_t * ptr);

void clearcpu_bitmask( cpu_bitmask_t * ptr, int cpu_num);

void print_bitmask( cpu_bitmask_t * ptr);

// must be in string.h
static inline int hex_to_bin(char ch)
{
        if ((ch >= '0') && (ch <= '9'))
                return ch - '0';
        ch = tolower(ch);
        if ((ch >= 'a') && (ch <= 'f'))
                return ch - 'a' + 10;
        return -1;
}
