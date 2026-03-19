#include <rthw.h>
#include <riscv-ops.h>

#define REG32(x) (*((volatile unsigned int *)((unsigned long)(x))))
#define GPIO1_PIN_LEVEL_REG	(0xd4019040)

static int GET_DDR_TYPE(void)
{
	/*
	 * using gpio52 to select the lp4 or pl5
	 * lp4: level high
	 * lp5: level low
	 */
	return 0;
	// return readl((unsigned int *)GPIO1_PIN_LEVEL_REG);
}

void lp45_enter_lp2(unsigned DDRC_BASE)
{
	unsigned int read_data;
	unsigned int CFG_BASE;
	CFG_BASE  = DDRC_BASE + 0x600000;
	//pre, enable quasi dynamic
	REG32(DDRC_BASE + 0x00010c80 ) = 0x00000000;

	//#2
	REG32(DDRC_BASE + 0x00010180) &= ~(0x11);//PWRCTL
	REG32(DDRC_BASE + 0x00010184) &= ~(0x1);//HWLPCTL

	//#3
	read_data = REG32(DDRC_BASE + 0x00010014);//STAT
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

	//#4
	REG32(DDRC_BASE + 0x00020090) &= ~(0x1);//PCTRL
	REG32(DDRC_BASE + 0x00021090) &= ~(0x1);
	REG32(DDRC_BASE + 0x00022090) &= ~(0x1);
	REG32(DDRC_BASE + 0x00023090) &= ~(0x1);
	REG32(DDRC_BASE + 0x00024090) &= ~(0x1);

	read_data = REG32(DDRC_BASE + 0x00020114);//PSTAT, seems the databook is wrong
	while(read_data & (0x0000001f)) {
		read_data = REG32(DDRC_BASE + 0x00020114);
	}

	/**
	 * read_data = REG32(DDRC_BASE + 0x00021114);
	 * REG32(MSG_PORT) = read_data;
	 * while(read_data != 0x0) {
	 * read_data = REG32(DDRC_BASE + 0x00021114);
	 * }
	 * 
	 * read_data = REG32(DDRC_BASE + 0x00022114);
	 * while(read_data != 0x0) {
	 * 	read_data = REG32(DDRC_BASE + 0x00022114);
	 * }
	 *
	 * read_data = REG32(DDRC_BASE + 0x00023114);
	 * while(read_data != 0x0) {
	 * 	read_data = REG32(DDRC_BASE + 0x00023114);
	 * }
	 *
	 * read_data = REG32(DDRC_BASE + 0x00024114);
	 * while(read_data != 0x0) {
	 * 	read_data = REG32(DDRC_BASE + 0x00024114);
	 * }
	 */

	//#5 we did not enable scrubber
	//#6 tmp
	REG32(DDRC_BASE + 0x00010b84) |= (0x1<<1);//OPCTRL1
	read_data = REG32(DDRC_BASE + 0x00010b88);//OPCTRLCAM
	while((read_data & 0x36000000) != 0x36000000) {
		read_data = REG32(DDRC_BASE + 0x00010b88);
	}
	
	//#7 tmp
	REG32(DDRC_BASE + 0x00010100) |= (0x1<<2);//DERATECTL0
	//#8 dummy
	REG32(DDRC_BASE + 0x00010508) |= (0x1<<31);//DFIUPD0
	//#9 dummy
	REG32(DDRC_BASE + 0x00010518) &= ~(0x1);//DFIPHYMSTR
	//#10 dummy
	REG32(DDRC_BASE + 0x00000a80) &= ~(0x1);//DQSOSCCTL0

	read_data = REG32(DDRC_BASE + 0x00010304);//DQSOSCSTAT0
	while((read_data & 0x00000007) != 0x00000000) {
		read_data = REG32(DDRC_BASE + 0x00010304);
	}

	//#11 tmp
	REG32(DDRC_BASE + 0x00010280) |= (0x1<<31);//ZQCTL0
	//#12 tmp
	REG32(DDRC_BASE + 0x00010500) &= ~(0x1<<4);//DFILPCFG0

	read_data = REG32(DDRC_BASE + 0x00010514);//DFISTAT
	while((read_data & 0x00000002) != 0x00000000) {
		read_data = REG32(DDRC_BASE + 0x00010514);
	}
	
	//#13 tmp
	REG32(DDRC_BASE + 0x00010500) &= ~(0x1<<20);
	read_data = REG32(DDRC_BASE + 0x00010514);
	while((read_data & 0x00000004) != 0x00000000) {
		read_data = REG32(DDRC_BASE + 0x00010514);
	}

	//#14 dummy
	REG32(DDRC_BASE + 0x00010180) &= ~(0x1<<9);
	//#15 tmp
	REG32(DDRC_BASE + 0x00010180) |= (0x1<<15);
	REG32(DDRC_BASE + 0x00010180) |= (0x1<<11);

	//#16 dummy
	unsigned int flag = 0x0;
	do{
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

		//#17
		read_data = REG32(DDRC_BASE + 0x00010014);
		flag = 0x1;
	} while (!(((read_data & 0x00007000) == 0x00001000) && ((read_data & 0x00000070) != 0x00000010)));

	//#18 tmp
	REG32(DDRC_BASE + 0x00010b84) |= 0x1;
	read_data = REG32(DDRC_BASE + 0x00010b88);
	while((read_data & 0x30000000) != 0x30000000) {
		read_data = REG32(DDRC_BASE + 0x00010b88);
	}

	//#19
	//#ifdef LPDDR4
	REG32(DDRC_BASE + 0x00010180) &= ~(0x1<<15);
	read_data = REG32(DDRC_BASE + 0x00010014);
	while((read_data & 0x00007000) != 0x00002000) {
		read_data = REG32(DDRC_BASE + 0x00010014);
	}

	//#endif
	//#20 tmp
	REG32(DDRC_BASE + 0x00010510) &= ~(0x1);
	//#21 dummy
	REG32(DDRC_BASE + 0x00010508) &= ~(0x1<<15);
	//#22
	REG32(DDRC_BASE + 0x00010510) |= (0x8<<8);//change to lp2
	//#23
	REG32(DDRC_BASE + 0x00010510) ^= (0x1<<14);
	//#24
	//REG32(DDRC_BASE + 0x00010008) = 0x1;
	//#25
	REG32(DDRC_BASE + 0x00010510) |= (0x1<<5);
	//#26
	read_data = REG32(DDRC_BASE + 0x00010514);
	while((read_data & 0x00000001) != 0x00000000) {
		read_data = REG32(DDRC_BASE + 0x00010514);
	}

	//#27 4266 to 6000
	/**
	 * REG32(0xd42828b0) &= ~(0x3f<<16); // div 0
	 * REG32(0xd42828b0) |= (0x1<<30) ; // sel 5
	 * REG32(0xd42828b0) |= (0x2<<19) | (0x3<<16) ; // sel 3, div 4
	 * REG32(0xd42828b0) |= (0x5<<19) | (0x0<<16) ; // sel 6, div 1
	 * REG32(0xd42828b0) |= (1<<25); // fc
	 * read_data = REG32(0xd42828b0);
	 * while ((read_data & 0x2000000) != 0x0){
	 * 	read_data = REG32(0xd42828b0);
	 * }
	 */
	REG32(CFG_BASE + 0xc) &= ~(0xff);
}

void lp45_exit_lp2(unsigned DDRC_BASE)
{
	unsigned int read_data;
	unsigned int CFG_BASE;

	CFG_BASE  = DDRC_BASE + 0x600000;

	REG32(CFG_BASE + 0xc) |= 0xff;
	//#28
	REG32(DDRC_BASE + 0x00010208) ^= (0x1<<4);
	//#29
	REG32(DDRC_BASE + 0x00010510) &= ~(0x1<<5);
	//#30
	//we do not enable dfi phy update
	//#31
	read_data = REG32(DDRC_BASE + 0x00010514);
	while((read_data & 0x00000001) != 0x00000001) {
		read_data = REG32(DDRC_BASE + 0x00010514);
	}

	//#32
	REG32(DDRC_BASE + 0x00010510) |= (0x1);
	//#33
	REG32(DDRC_BASE + 0x00010288) |= (0x1);
	//#34
	if(GET_DDR_TYPE()) {
		REG32(DDRC_BASE + 0x00010180) |= (0x1<<15);
		REG32(DDRC_BASE + 0x00010180) &= ~(0x1<<11);

		read_data = REG32(DDRC_BASE + 0x00010014);
		while((read_data & 0x00007000) != 0x00003000) {
			read_data = REG32(DDRC_BASE + 0x00010014);
		}
	}

	//#35
	REG32(DDRC_BASE + 0x00010b8c) |= (0x1<<16);
	read_data = REG32(DDRC_BASE + 0x00010b90);
	while((read_data & 0x00010000) != 0x00000000) {
		read_data = REG32(DDRC_BASE + 0x00010b90);
	}

	//#36
	REG32(DDRC_BASE + 0x00010288) &= ~(0x1);
	//#37
	if(!GET_DDR_TYPE()) {
	REG32(DDRC_BASE + 0x00010180) &= ~(0x1<<11);//15
	}

	//#38
	REG32(DDRC_BASE + 0x00010180) &= ~(0x1<<15);//15
	read_data = REG32(DDRC_BASE + 0x00010014);
	while((read_data & 0x00007000) != 0x00000000) {
		read_data = REG32(DDRC_BASE + 0x00010014);
	}

	//#39
	REG32(DDRC_BASE + 0x00010500) |= (0x1<<4)|(0x1<<20);//12,13
	//#40
	REG32(DDRC_BASE + 0x00010b84) &= ~(0x1);//18
	//#41
	//#42
	REG32(DDRC_BASE + 0x00010b84) &= ~(0x1<<1);//6
	//#43
	REG32(DDRC_BASE + 0x00010100) &= ~(0x1<<2);//7
	//#44
	//#8 is dummy
	//#45
	//#9 is dummy
	//#46
	//#10 is dummy
	//#47
	REG32(DDRC_BASE + 0x00010280) |= (0x1<<31);//11
	//#48
	//REG32(DDRC_BASE + 0x00010180) |= (0x10);
	//10184 is dummy
	//#49
	//#14 is dummy
	//#50
	REG32(DDRC_BASE + 0x00020090) |= (0x1);
	REG32(DDRC_BASE + 0x00021090) |= (0x1);
	REG32(DDRC_BASE + 0x00022090) |= (0x1);
	REG32(DDRC_BASE + 0x00023090) |= (0x1);
	REG32(DDRC_BASE + 0x00024090) |= (0x1);
	//post, disable quasi dynamic
	REG32(DDRC_BASE + 0x00010c80 ) = 0x00000001;
	//init_snps_mr(DDRC_BASE, 0x00001005);
}

