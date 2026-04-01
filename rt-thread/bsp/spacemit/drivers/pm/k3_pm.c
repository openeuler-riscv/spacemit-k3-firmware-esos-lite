#include <rthw.h>
#include <drivers/pm.h>
#include <rtconfig.h>
#include <rtthread.h>
#include <riscv_sleep.h>
#include <riscv-ops.h>
#include <riscv_encoding.h>
#include <clint.h>
#include <riscv-plic.h>
#include <spacemit_sdk_soc.h>
#include <register_defination.h>

static struct rt_thread rt_lpm_thread;
ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t rt_lpw_stack[1024];
static struct rt_semaphore system_lpm_sem;

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

static int __suspend_asm_finish(rt_ubase_t arg, rt_ubase_t entry, rt_ubase_t context)
{
	unsigned int val;
	unsigned int read_data;
	unsigned int CFG_BASE;
	unsigned int DDRC_BASE;
	unsigned int retry = 100000;

	/* set the rcpu entery point */
	writel(entry & 0xffffffff, (void *)RCPU_CORE0_BOOT_ENTRY_LO);
	writel((entry >> 32) & 0xffffffff, (void *)RCPU_CORE0_BOOT_ENTRY_HI);

	/* must add fence */
	asm volatile ("fence iorw, iorw");

	/* channel 0 enter self-refresh */
	{
		DDRC_BASE = 0xcb000000; 
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

	/* channel 1 enter self-refresh */
	{
		DDRC_BASE = 0xcc000000; 
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

	/* vote per */
	REG32((unsigned int *)APCR_PER_VETE_REG) |= APCR_PER_DEFAULT_VATE_VALUE;

	/* enable wakeup source */
	REG32((unsigned int *)AWUCRM_REG) |= (1 << PMIC_WKUP_BIT_OFFSET) |
					     (1 << USB_RAUD_APAUD_WKUP_BIT_OFFSET);

	/* SCCR */
	REG32((unsigned int *)SCCR_REG) = 0x5;

	REG32((unsigned int *)APB_SPARE4_REG) = 0xe31450e;

	/* set pmu debug */
	REG32((unsigned int *)PMU_DEBUG_REG) |=0X984024;

	/* audio main pmu vote */
	REG32((unsigned int *)AUDIO_VOTE_FOR_MAIN_PMU) |=0xff;

	/* audio low power */
	REG32((unsigned int *)AUDIO_PMU_VOTE_REG) |= ((1 << 0) | (1 << 2));

	/* devote core powrdown */
	REG32((unsigned int *)RT24_CORE0_IDLE_CFG_REG) |= ((1 << 0)  | (1 << 1));

	/* wait soc top enter D2 */
	val = REG32((unsigned int *)RT24_PMU_STATUS);
	while (1) {
		if (((val >> 8) & 0xf) == 0x6)
			break;

		val = REG32((unsigned int *)RT24_PMU_STATUS);

		if ((--retry) == 0)
			break;
	}

	/* clear the pending */
	REG32((unsigned int *)SOC_TOP_D2_LP_CTRL) |= (1 << 2);

	while (1) {
		val = REG32((unsigned int *)SOC_TOP_D2_LP_CTRL);
		if ((val & 0x10) == 0)
			break;
	}

	REG32((unsigned int *)SOC_TOP_D2_LP_CTRL) &= ~(1 << 2);

	/*
	 * increase the priority of D2 wakeup interrupt
	 */
	__plic_set_priority(D2_WAKEUP_EN_IRQ_NUM, 2);
	/* Enable D2 wakeup interrupt */
	__plic_irq_enable(D2_WAKEUP_EN_IRQ_NUM);
	/* Enable M2 exit interrupt */
	__plic_irq_enable(AP_C0_M2_EXIT_INT_NUM);

	asm volatile ("fence iorw, iorw");
	asm volatile ("fence.i");

	/* wfi */
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile ("wfi");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");

	retry = 100000;

	/* wait top wakeup */
	while (1) {
		val = REG32((unsigned int *)SOC_TOP_D2_LP_CTRL);
		if (val & (1 << 5))
			break;

		if ((--retry) == 0)
			break;
	}

	/* clear the wakeup en pending, then the ap will wakeup */
	REG32((unsigned int *)SOC_TOP_D2_LP_CTRL) |= (1 << 1);

	for (int i = 0; i < 10; ++i) {};

	REG32((unsigned int *)SOC_TOP_D2_LP_CTRL) &= ~(1 << 1);

	retry = 100000;

	while (1) {
		val = REG32((unsigned int *)RT24_PMU_STATUS);
		if (((val >> 8) & 0xf) == 0xd) {
			/* Consume the D2 wakeup interruption */
			val = __plic_irq_claim();
			__plic_irq_complete(val);
			break;
		}

		if ((--retry) == 0)
			break;
	}

	/* disable plic D2 ext interrupt */
	__plic_irq_disable(D2_WAKEUP_EN_IRQ_NUM);

	/* audio main pmu de-vote */
	REG32((unsigned int *)AUDIO_VOTE_FOR_MAIN_PMU) &= ~(0xff);

	/* devote core powrdown */
	REG32((unsigned int *)RT24_CORE0_IDLE_CFG_REG) &= ~((1 << 0) | (1 << 1));

	/* audio low power */
	REG32((unsigned int *)AUDIO_PMU_VOTE_REG) &= ~((1 << 0) | (1 << 2));

	/* wait core exit from M2 */
	while (1) {
		val = REG32((unsigned int *)AP_C0_M2_INT_EN_REG);
		if (val & (1 << 5))
			break;
	}

	/* disable M2 exit interrupt which will be consumed by big-os */
	__plic_irq_disable(AP_C0_M2_EXIT_INT_NUM);

	/* channel0 exit ddr self-refresh */
	{
		DDRC_BASE = 0xcb000000;	
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

	/* channel1 exit ddr self-refresh */
	{
		DDRC_BASE = 0xcc000000;	
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

	/* de-vote */
	REG32((unsigned int *)APCR_PER_VETE_REG) &= ~APCR_PER_DEFAULT_VATE_VALUE;

	return RT_EOK;
}

static void suspend_save_csrs(struct suspend_context *context)
{
	context->mscratch = read_csr(mscratch);
	context->mie = read_csr(mie);
	context->mtvec = read_csr(mtvec);
}

static void suspend_restore_csrs(struct suspend_context *context)
{
	write_csr(mscratch, context->mscratch);
	write_csr(mtvec, context->mtvec);
	write_csr(mie, context->mie);
}

extern int __cpu_suspend_enter(rt_ubase_t context);
extern int __cpu_resume_enter_pre(rt_ubase_t context);
extern int __cpu_resume_enter(rt_ubase_t context);

struct suspend_context context = { 0 };

static void __spacemit_wakeup_asm(void)
{
	unsigned int val;
	unsigned int read_data;
	unsigned int CFG_BASE;
	unsigned int DDRC_BASE;
	unsigned int retry = 100000;

	/* wait top wakeup */
	while (1) {
		val = REG32((unsigned int *)SOC_TOP_D2_LP_CTRL);
		if (val & (1 << 5))
			break;
		if ((--retry) == 0)
			break;
	}

	/* clear the wakeup en pending, then the ap will wakeup */
	REG32((unsigned int *)SOC_TOP_D2_LP_CTRL) |= (1 << 1);

	for (int i = 0; i < 10; ++i) {};

	REG32((unsigned int *)SOC_TOP_D2_LP_CTRL) &= ~(1 << 1);

	retry = 100000;
	while (1) {
		val = REG32((unsigned int *)RT24_PMU_STATUS);
		if (((val >> 8) & 0xf) == 0xd) {
			/* Consume the interruption */
			val = __plic_irq_claim();
			__plic_irq_complete(val);
			break;
		}
		if ((--retry) == 0)
			break;
	}

	/* disable plic D2 ext interrupt */
	__plic_irq_disable(D2_WAKEUP_EN_IRQ_NUM);

	/* audio main pmu de-vote */
	REG32((unsigned int *)AUDIO_VOTE_FOR_MAIN_PMU) &= ~(0xff);

	/* devote core powrdown */
	REG32((unsigned int *)RT24_CORE0_IDLE_CFG_REG) &= ~((1 << 0) | (1 << 1));

	/* audio low power */
	REG32((unsigned int *)AUDIO_PMU_VOTE_REG) &= ~((1 << 0) | (1 << 2));

	/* wait core exit from M2 */
	while (1) {
		val = REG32((unsigned int *)AP_C0_M2_INT_EN_REG);
		if (val & (1 << 5))
			break;
	}

	/* disable the M2 exit interrupt and will be consumed by big-os */
	__plic_irq_disable(AP_C0_M2_EXIT_INT_NUM);

	/* channel0 exit ddr self-refresh */
	{
		DDRC_BASE = 0xcb000000;	
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

	/* channel1 exit ddr self-refresh */
	{
		DDRC_BASE = 0xcc000000;	
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

	/* de-vote */
	REG32((unsigned int *)APCR_PER_VETE_REG) &= ~APCR_PER_DEFAULT_VATE_VALUE;

	__cpu_resume_enter(0);
}

int cpu_suspend(rt_ubase_t arg,
                int (*finish)(rt_ubase_t arg,
                              rt_ubase_t entry,
                              rt_ubase_t context))
{
	int rc = 0;

	/* Finisher should be non-NULL */
	if (!finish)
		return -RT_EINVAL;

	/* Save additional CSRs*/
	suspend_save_csrs(&context);

	/* Save context on stack */
	if (__cpu_suspend_enter((unsigned long)&context)) {
		/* Call the finisher */
		rc = finish(arg, (rt_ubase_t)__spacemit_wakeup_asm, (rt_ubase_t)&context);

		/*
		 * Should never reach here, unless the suspend finisher
		 * fails. Successful cpu_suspend() should return from
		 * __cpu_resume_entry()
		 */
		if (!rc)
			rc = -RT_EINVAL;
	}

	/* Restore additional CSRs */
	suspend_restore_csrs(&context);

	return rc;
}

extern void rt_hw_eclic_save(void);
extern void rt_hw_eclic_restore(void);

/**
 * This function will put n308 into sleep mode.
 *
 * @param pm pointer to power manage structure
 */
static void sleep(struct rt_pm *pm, uint8_t mode)
{
	unsigned int val;
	rt_uint64_t time;

	switch (mode)
	{
	case PM_SLEEP_MODE_NONE:
	break;

	case PM_SLEEP_MODE_IDLE:
	break;

	case PM_SLEEP_MODE_LIGHT:
	break;

	case PM_SLEEP_MODE_DEEP:
		/* save the plic configuration */
		rt_hw_eclic_save();

		/* disable the clint timer */
		time = SysTimer_GetLoadValue();
		SysTimer_SetCompareValue(0xffffffffffffffff);
		/* clear the timer pending */
		clear_csr(mip, MIP_MTIP);

		cpu_suspend(0, __suspend_asm_finish);

		/* enable the clint timer */
		SysTimer_SetCompareValue(time);

		/* restore the plic configuration */
		rt_hw_eclic_restore();

		rt_pm_request(RT_PM_DEFAULT_SLEEP_MODE);

		rt_sem_release(&system_lpm_sem);
	break;

	case PM_SLEEP_MODE_STANDBY:
	break;

	case PM_SLEEP_MODE_SHUTDOWN:
	break;

	default:
		RT_ASSERT(0);
	break;
	}
}

static void run(struct rt_pm *pm, uint8_t mode)
{
}

/**
 * This function start the timer of pm
 *
 * @param pm Pointer to power manage structure
 * @param timeout How many OS Ticks that MCU can sleep
 */
static void pm_timer_start(struct rt_pm *pm, rt_uint32_t timeout)
{
	RT_ASSERT(pm != RT_NULL);
	RT_ASSERT(timeout > 0);
}

/**
 * This function stop the timer of pm
 *
 * @param pm Pointer to power manage structure
 */
static void pm_timer_stop(struct rt_pm *pm)
{
	RT_ASSERT(pm != RT_NULL);
}

/**
 * This function calculate how many OS Ticks that MCU have suspended
 *
 * @param pm Pointer to power manage structure
 *
 * @return OS Ticks
 */
static rt_tick_t pm_timer_get_tick(struct rt_pm *pm)
{
	return 0;
}

extern unsigned long __liteos_param_store[];

static void rt_thread_system_lpm_entry(void *parameter)
{
	rt_base_t level;
	typedef void (*__entry)(void);
	__entry ptr;

	while (1) {
		rt_sem_take(&system_lpm_sem, RT_WAITING_FOREVER);

		/* disable the irq */
		level = rt_hw_interrupt_disable();

		/* jump to big os */
		ptr = (__entry)(__liteos_param_store[0]);
		ptr();

	}
}

int rt_hw_k3_pm_init(void)
{
	rt_uint8_t timer_mask = 0;
	audio_wakeup_en_t *dwkup = (audio_wakeup_en_t *)AUDIO_WAKEUP_EN_REG;
	soc_top_d2_lp_ctrl *lpd2 = (soc_top_d2_lp_ctrl *)SOC_TOP_D2_LP_CTRL;

	static const struct rt_pm_ops _ops = {
		sleep,
		run,
		pm_timer_start,
		pm_timer_stop,
		pm_timer_get_tick
	};

	/* enable en wakeup interrupt */
	dwkup->bits.d2_exit_en = 1;
	dwkup->bits.d2_enter_en = 1;
	lpd2->bits.rcpu_ctrl_soc_top_d2_lp_en = 1;

	/* create a thread to jump to big-os */
	rt_sem_init(&system_lpm_sem, "slpm", 0, RT_IPC_FLAG_FIFO);

	/* create defunct thread */
	rt_thread_init(&rt_lpm_thread,
			"tlpm",
			rt_thread_system_lpm_entry,
			RT_NULL,
			rt_lpw_stack,
			sizeof(rt_lpw_stack),
			RT_THREAD_PRIORITY_MAX / 3,
			32);
	/* startup */
	rt_thread_startup(&rt_lpm_thread);

	/* initialize system pm module */
	rt_system_pm_init(&_ops, timer_mask, RT_NULL);

	return 0;
}
INIT_COMPONENT_EXPORT(rt_hw_k3_pm_init);
