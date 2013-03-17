#define cpu_relax() __asm__ ("pause":::"memory")
#define barrier() __asm__ __volatile__("":::"memory")

enum {
	REG_RAX = 0,
	REG_RDI,
	REG_RSI,
	REG_RDX,
	REG_R8,
	REG_R9,
	NUM_REGS
};

struct syscall_packet {
	int64_t regs[NUM_REGS];
	int64_t ctl;
} __attribute__((packed)) __attribute__((aligned(64)));

#define CTL_READY 0x1

static inline void set_ready(volatile struct syscall_packet *pkt)
{
	pkt->ctl |= CTL_READY;
}

static inline void clear_ready(volatile struct syscall_packet *pkt)
{
	pkt->ctl &= ~(CTL_READY);
}

static inline int is_ready(volatile struct syscall_packet *pkt)
{
	return (pkt->ctl & CTL_READY);
}

