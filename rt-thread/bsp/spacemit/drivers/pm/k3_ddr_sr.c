#include <rthw.h>
#include <riscv-ops.h>

#define REG32(x) (*((volatile unsigned int *)((unsigned long)(x))))
#define GPIO1_PIN_LEVEL_REG	(0xd4019040)

static inline int GET_DDR_TYPE(void)
{
	/*
	 * using gpio52 to select the lp4 or pl5
	 * lp4: level high
	 * lp5: level low
	 */
	return ((readl((unsigned int *)GPIO1_PIN_LEVEL_REG) >> 20) & 0x1);
}

void lp45_enter_lp2(unsigned DDRC_BASE)
{
	unsigned int read_data;
	unsigned int CFG_BASE;

	CFG_BASE  = DDRC_BASE + 0x600000;

	/* pre, enable quasi dynamic */
	REG32(DDRC_BASE + 0x00010c80 ) = 0x00000000;

	REG32(DDRC_BASE + 0x00010180) &= ~(0x11);
	REG32(DDRC_BASE + 0x00010184) &= ~(0x1);

	read_data = REG32(DDRC_BASE + 0x00010014);
	while((read_data & 0x00000003) != 0x00000001) {
		read_data = REG32(DDRC_BASE + 0x00010014);
	}

	read_data = REG32(DDRC_BASE + 0x00010014);
	while((read_data & 0x00000003) != 0x00000001) {
		read_data = REG32(DDRC_BASE + 0x00010014);
	}

	read_data = REG32(DDRC_BASE + 0x00010014);
	while((read_data & 0x00000003) != 0x00000001) {
		read_data = REG32(DDRC_BASE + 0x00010014);
	}

	REG32(DDRC_BASE + 0x00020090) &= ~(0x1);
	REG32(DDRC_BASE + 0x00021090) &= ~(0x1);
	REG32(DDRC_BASE + 0x00022090) &= ~(0x1);
	REG32(DDRC_BASE + 0x00023090) &= ~(0x1);
	REG32(DDRC_BASE + 0x00024090) &= ~(0x1);

	read_data = REG32(DDRC_BASE + 0x00020114);
	while(read_data & (0x0000001f)) {
		read_data = REG32(DDRC_BASE + 0x00020114);
	}

	REG32(DDRC_BASE + 0x00010b84) |= (0x1<<1);
	read_data = REG32(DDRC_BASE + 0x00010b88);
	while((read_data & 0x36000000) != 0x36000000) {
		read_data = REG32(DDRC_BASE + 0x00010b88);
	}

	REG32(DDRC_BASE + 0x00010100) |= (0x1<<2);

	REG32(DDRC_BASE + 0x00010508) |= (0x1<<31);
	REG32(DDRC_BASE + 0x00010518) &= ~(0x1);
	REG32(DDRC_BASE + 0x00000a80) &= ~(0x1);

	read_data = REG32(DDRC_BASE + 0x00010304);
	while((read_data & 0x00000007) != 0x00000000) {
		read_data = REG32(DDRC_BASE + 0x00010304);
	}

	REG32(DDRC_BASE + 0x00010280) |= (0x1<<31);
	REG32(DDRC_BASE + 0x00010500) &= ~(0x1<<4);

	read_data = REG32(DDRC_BASE + 0x00010514);
	while((read_data & 0x00000002) != 0x00000000) {
		read_data = REG32(DDRC_BASE + 0x00010514);
	}

	REG32(DDRC_BASE + 0x00010500) &= ~(0x1<<20);
	read_data = REG32(DDRC_BASE + 0x00010514);
	while((read_data & 0x00000004) != 0x00000000) {
		read_data = REG32(DDRC_BASE + 0x00010514);
	}

	REG32(DDRC_BASE + 0x00010180) &= ~(0x1<<9);
	REG32(DDRC_BASE + 0x00010180) |= (0x1<<15);
	REG32(DDRC_BASE + 0x00010180) |= (0x1<<11);

	unsigned int flag = 0x0;
	do {
		if(flag == 0x1){
			REG32(DDRC_BASE + 0x00010180) &= ~((0x1<<11)|(0x1<<15));
			read_data = REG32(DDRC_BASE + 0x00010014);
			while((read_data & 0x00007000) != 0x00000000) {
			      read_data = REG32(DDRC_BASE + 0x00010014);
			}
		}

		read_data = REG32(DDRC_BASE + 0x00010014);
		while((read_data & 0x00007000) == 0x00000000) {
			read_data = REG32(DDRC_BASE + 0x00010014);
		}

		read_data = REG32(DDRC_BASE + 0x00010014);
		flag = 0x1;
	} while (!(((read_data & 0x00007000) == 0x00001000) && ((read_data & 0x00000070) != 0x00000010)));

	REG32(DDRC_BASE + 0x00010b84) |= 0x1;
	read_data = REG32(DDRC_BASE + 0x00010b88);
	while((read_data & 0x30000000) != 0x30000000) {
		read_data = REG32(DDRC_BASE + 0x00010b88);
	}

	REG32(DDRC_BASE + 0x00010180) &= ~(0x1<<15);
	read_data = REG32(DDRC_BASE + 0x00010014);
	while((read_data & 0x00007000) != 0x00002000) {
		read_data = REG32(DDRC_BASE + 0x00010014);
	}

	REG32(DDRC_BASE + 0x00010510) &= ~(0x1);
	REG32(DDRC_BASE + 0x00010508) &= ~(0x1<<15);
	REG32(DDRC_BASE + 0x00010510) |= (0x8<<8);
	REG32(DDRC_BASE + 0x00010510) ^= (0x1<<14);
	REG32(DDRC_BASE + 0x00010510) |= (0x1<<5);

	read_data = REG32(DDRC_BASE + 0x00010514);
	while((read_data & 0x00000001) != 0x00000000) {
		read_data = REG32(DDRC_BASE + 0x00010514);
	}

	REG32(CFG_BASE + 0xc) &= ~(0xff);
}

void lp45_exit_lp2(unsigned DDRC_BASE)
{
	unsigned int read_data;
	unsigned int CFG_BASE;

	CFG_BASE  = DDRC_BASE + 0x600000;

	REG32(CFG_BASE + 0xc) |= 0xff;
	REG32(DDRC_BASE + 0x00010208) ^= (0x1<<4);
	REG32(DDRC_BASE + 0x00010510) &= ~(0x1<<5);

	read_data = REG32(DDRC_BASE + 0x00010514);
	while((read_data & 0x00000001) != 0x00000001) {
		read_data = REG32(DDRC_BASE + 0x00010514);
	}

	REG32(DDRC_BASE + 0x00010510) |= (0x1);
	REG32(DDRC_BASE + 0x00010288) |= (0x1);

	if(GET_DDR_TYPE()) {
		REG32(DDRC_BASE + 0x00010180) |= (0x1<<15);
		REG32(DDRC_BASE + 0x00010180) &= ~(0x1<<11);

		read_data = REG32(DDRC_BASE + 0x00010014);
		while((read_data & 0x00007000) != 0x00003000) {
			read_data = REG32(DDRC_BASE + 0x00010014);
		}
	}

	REG32(DDRC_BASE + 0x00010b8c) |= (0x1<<16);

	read_data = REG32(DDRC_BASE + 0x00010b90);
	while((read_data & 0x00010000) != 0x00000000) {
		read_data = REG32(DDRC_BASE + 0x00010b90);
	}

	REG32(DDRC_BASE + 0x00010288) &= ~(0x1);

	if(!GET_DDR_TYPE()) {
		REG32(DDRC_BASE + 0x00010180) &= ~(0x1<<11);
	}

	REG32(DDRC_BASE + 0x00010180) &= ~(0x1<<15);

	read_data = REG32(DDRC_BASE + 0x00010014);
	while((read_data & 0x00007000) != 0x00000000) {
		read_data = REG32(DDRC_BASE + 0x00010014);
	}

	REG32(DDRC_BASE + 0x00010500) |= (0x1<<4)|(0x1<<20);
	REG32(DDRC_BASE + 0x00010b84) &= ~(0x1);
	REG32(DDRC_BASE + 0x00010b84) &= ~(0x1<<1);
	REG32(DDRC_BASE + 0x00010100) &= ~(0x1<<2);
	REG32(DDRC_BASE + 0x00010280) |= (0x1<<31);
	REG32(DDRC_BASE + 0x00020090) |= (0x1);
	REG32(DDRC_BASE + 0x00021090) |= (0x1);
	REG32(DDRC_BASE + 0x00022090) |= (0x1);
	REG32(DDRC_BASE + 0x00023090) |= (0x1);
	REG32(DDRC_BASE + 0x00024090) |= (0x1);
	/* post, disable quasi dynamic */
	REG32(DDRC_BASE + 0x00010c80 ) = 0x00000001;
}

