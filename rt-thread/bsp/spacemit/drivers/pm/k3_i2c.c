// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023 Spacemit
 */

#include <rtthread.h>
#include <rtconfig.h>
#include <rthw.h>
#include <riscv-ops.h>
#include "k3_i2c.h"

/* All transfers are described by this data structure */
struct spacemit_i2c_msg {
	rt_uint8_t condition;
	rt_uint8_t acknack;
	rt_uint8_t direction;
	rt_uint8_t data;
};

struct spacemit_i2c {
	rt_uint32_t icr;
	rt_uint32_t isr;
	rt_uint32_t isar;
	rt_uint32_t idbr;
	rt_uint32_t ilcr;
	rt_uint32_t iwcr;
	rt_uint32_t irst_cyc;
	rt_uint32_t ibmr;
};

/*
 * i2c_reset: - reset the host controller
 *
 */
static void i2c_reset(struct spacemit_i2c *base, rt_bool_t sda_glitch_nofix)
{
	rt_uint32_t icr_mode;

	/* Save bus mode (standard or fast speed) for later use */
	icr_mode = readl(&base->icr) & ICR_MODE_MASK;
	writel(readl(&base->icr) & ~ICR_IUE, &base->icr); /* disable unit */
	writel(readl(&base->icr) | ICR_UR, &base->icr);	  /* reset the unit */
	rt_hw_us_delay(100);
	writel(readl(&base->icr) & ~ICR_IUE, &base->icr); /* disable unit */

#ifdef CONFIG_SYS_I2C_SLAVE
	writel(CONFIG_SYS_I2C_SLAVE, &base->isar); /* set our slave address */
#else
	writel(0x00, &base->isar); /* set our slave address */
#endif
	/* set control reg values */
	writel(I2C_ICR_INIT | icr_mode, &base->icr);
	writel(I2C_ISR_INIT, &base->isr); /* set clear interrupt bits */
	if (sda_glitch_nofix)
		writel(readl(&base->irst_cyc) | IRCR_SDA_GLITCH_NOFIX,
		       &base->irst_cyc);
	writel(readl(&base->icr) | ICR_IUE, &base->icr); /* enable unit */
	rt_hw_us_delay(1);
}

/*
 * i2c_isr_set_cleared: - wait until certain bits of the I2C status register
 *	                  are set and cleared
 *
 * @return: 1 in case of success, 0 means timeout (no match within 10 ms).
 */
static int i2c_isr_set_cleared(struct spacemit_i2c *base, unsigned long set_mask,
			       unsigned long cleared_mask)
{
	int timeout = 1000, isr;

	do {
		isr = readl(&base->isr);
		/* udelay(10); */
		/* this delay time can't exceed the watchog timeout period */
		rt_hw_us_delay(10);
		if (timeout-- < 0)
			return 0;
	} while (((isr & set_mask) != set_mask)
		|| ((isr & cleared_mask) != 0));

	return 1;
}

/*
 * i2c_transfer: - Transfer one byte over the i2c bus
 *
 * This function can tranfer a byte over the i2c bus in both directions.
 * It is used by the public API functions.
 *
 * @return:  0: transfer successful
 *          -1: message is empty
 *          -2: transmit timeout
 *          -3: ACK missing
 *          -4: receive timeout
 *          -5: illegal parameters
 *          -6: bus is busy and couldn't be aquired
 */
static int i2c_transfer(struct spacemit_i2c *base, struct spacemit_i2c_msg *msg,
			rt_bool_t sda_glitch_nofix)
{
	int ret;

	if (!msg)
		goto transfer_error_msg_empty;

	switch (msg->direction) {
	case I2C_WRITE:
		/* check if bus is not busy */
		if (!i2c_isr_set_cleared(base, 0, ISR_IBB))
			goto transfer_error_bus_busy;

		/* start transmission */
		writel(readl(&base->icr) & ~ICR_START, &base->icr);
		writel(readl(&base->icr) & ~ICR_STOP, &base->icr);
		writel(msg->data, &base->idbr);
		if (msg->condition == I2C_COND_START)
			writel(readl(&base->icr) | ICR_START, &base->icr);
		if (msg->condition == I2C_COND_STOP)
			writel(readl(&base->icr) | ICR_STOP, &base->icr);
		if (msg->acknack == I2C_ACKNAK_SENDNAK)
			writel(readl(&base->icr) | ICR_ACKNAK, &base->icr);
		if (msg->acknack == I2C_ACKNAK_SENDACK)
			writel(readl(&base->icr) & ~ICR_ACKNAK, &base->icr);
		writel(readl(&base->icr) & ~ICR_ALDIE, &base->icr);
		writel(readl(&base->icr) | ICR_TB, &base->icr);

		/* transmit register empty? */
		if (!i2c_isr_set_cleared(base, ISR_ITE, 0))
			goto transfer_error_transmit_timeout;

		/* clear 'transmit empty' state */
		writel(readl(&base->isr) | ISR_ITE, &base->isr);

		/* wait for ACK from slave */
		if (msg->acknack == I2C_ACKNAK_WAITACK)
			if (!i2c_isr_set_cleared(base, 0, ISR_ACKNAK))
				goto transfer_error_ack_missing;
		break;

	case I2C_READ:

		/* check if bus is not busy */
		if (!i2c_isr_set_cleared(base, 0, ISR_IBB))
			goto transfer_error_bus_busy;

		/* start receive */
		writel(readl(&base->icr) & ~ICR_START, &base->icr);
		writel(readl(&base->icr) & ~ICR_STOP, &base->icr);
		if (msg->condition == I2C_COND_START)
			writel(readl(&base->icr) | ICR_START, &base->icr);
		if (msg->condition == I2C_COND_STOP)
			writel(readl(&base->icr) | ICR_STOP, &base->icr);
		if (msg->acknack == I2C_ACKNAK_SENDNAK)
			writel(readl(&base->icr) | ICR_ACKNAK, &base->icr);
		if (msg->acknack == I2C_ACKNAK_SENDACK)
			writel(readl(&base->icr) & ~ICR_ACKNAK, &base->icr);
		writel(readl(&base->icr) & ~ICR_ALDIE, &base->icr);
		writel(readl(&base->icr) | ICR_TB, &base->icr);

		/* receive register full? */
		if (!i2c_isr_set_cleared(base, ISR_IRF, 0))
			goto transfer_error_receive_timeout;

		msg->data = readl(&base->idbr);

		/* clear 'receive empty' state */
		writel(readl(&base->isr) | ISR_IRF, &base->isr);
		break;
	default:
		goto transfer_error_illegal_param;
	}

	return 0;

transfer_error_msg_empty:
//	rt_kprintf("i2c_transfer: error: 'msg' is empty\n");
	ret = -1;
	goto i2c_transfer_finish;

transfer_error_transmit_timeout:
//	rt_kprintf("i2c_transfer: error: transmit timeout\n");
	ret = -2;
	goto i2c_transfer_finish;

transfer_error_ack_missing:
//	rt_kprintf("i2c_transfer: error: ACK missing\n");
	ret = -3;
	goto i2c_transfer_finish;

transfer_error_receive_timeout:
//	rt_kprintf("i2c_transfer: error: receive timeout\n");
	ret = -4;
	goto i2c_transfer_finish;

transfer_error_illegal_param:
//	rt_kprintf("i2c_transfer: error: illegal parameters\n");
	ret = -5;
	goto i2c_transfer_finish;

transfer_error_bus_busy:
//	rt_kprintf("i2c_transfer: error: bus is busy\n");
	ret = -6;
	goto i2c_transfer_finish;

i2c_transfer_finish:
//	rt_kprintf("i2c_transfer: ISR: 0x%04x\n", readl(&base->isr));
	i2c_reset(base, sda_glitch_nofix);
	return ret;
}

static int __i2c_read(struct spacemit_i2c *base, rt_uint8_t chip, rt_uint8_t *addr, int alen,
		      rt_uint8_t *buffer, int len, rt_bool_t sda_glitch_nofix)
{
	struct spacemit_i2c_msg msg;

//	rt_kprintf("i2c_read(chip=0x%02x, len=0x%02x)\n", chip, len);

	if (len == 0) {
//		rt_kprintf("reading zero byte is invalid\n");
		return -RT_EINVAL;
	}

	i2c_reset(base, sda_glitch_nofix);

	/* dummy chip address write */
//	rt_kprintf("i2c_read: dummy chip address write\n");
	msg.condition = I2C_COND_START;
	msg.acknack   = I2C_ACKNAK_WAITACK;
	msg.direction = I2C_WRITE;
	msg.data = (chip << 1);
	msg.data &= 0xFE;
	if (i2c_transfer(base, &msg, sda_glitch_nofix))
		return -1;

	/*
	 * send memory address bytes;
	 * alen defines how much bytes we have to send.
	 */
	while (--alen >= 0) {
//		rt_kprintf("i2c_read: send address byte %02x (alen=%d)\n",
//		      *addr, alen);
		msg.condition = I2C_COND_NORMAL;
		msg.acknack   = I2C_ACKNAK_WAITACK;
		msg.direction = I2C_WRITE;
		msg.data      = addr[alen];
		if (i2c_transfer(base, &msg, sda_glitch_nofix))
			return -1;
	}

	/* start read sequence */
//	rt_kprintf("i2c_read: start read sequence\n");
	msg.condition = I2C_COND_START;
	msg.acknack   = I2C_ACKNAK_WAITACK;
	msg.direction = I2C_WRITE;
	msg.data      = (chip << 1);
	msg.data     |= 0x01;
	if (i2c_transfer(base, &msg, sda_glitch_nofix))
		return -1;

	/* read bytes; send NACK at last byte */
	while (len--) {
		if (len == 0) {
			msg.condition = I2C_COND_STOP;
			msg.acknack   = I2C_ACKNAK_SENDNAK;
		} else {
			msg.condition = I2C_COND_NORMAL;
			msg.acknack   = I2C_ACKNAK_SENDACK;
		}

		msg.direction = I2C_READ;
		msg.data      = 0x00;
		if (i2c_transfer(base, &msg, sda_glitch_nofix))
			return -1;

		*buffer = msg.data;
//		rt_kprintf("i2c_read: reading byte (%p)=0x%02x\n",
//		      buffer, *buffer);
		buffer++;
	}

	i2c_reset(base, sda_glitch_nofix);

	return 0;
}

static int __i2c_write(struct spacemit_i2c *base, rt_uint8_t chip, rt_uint8_t *addr, int alen,
		       rt_uint8_t *buffer, int len, rt_bool_t sda_glitch_nofix)
{
	struct spacemit_i2c_msg msg;

//	rt_kprintf("i2c_write(chip=0x%02x, len=0x%02x)\n", chip, len);

	i2c_reset(base, sda_glitch_nofix);

	/* chip address write */
//	rt_kprintf("i2c_write: chip address write\n");
	msg.condition = I2C_COND_START;
	msg.acknack   = I2C_ACKNAK_WAITACK;
	msg.direction = I2C_WRITE;
	msg.data = (chip << 1);
	msg.data &= 0xFE;
	if (i2c_transfer(base, &msg, sda_glitch_nofix))
		return -1;

	/*
	 * send memory address bytes;
	 * alen defines how much bytes we have to send.
	 */
	while (--alen >= 0) {
//		rt_kprintf("i2c_read: send address byte %02x (alen=%d)\n",
//		      *addr, alen);
		msg.condition = I2C_COND_NORMAL;
		msg.acknack   = I2C_ACKNAK_WAITACK;
		msg.direction = I2C_WRITE;
		msg.data      = addr[alen];
		if (i2c_transfer(base, &msg, sda_glitch_nofix))
			return -1;
	}

	/* write bytes; send NACK at last byte */
	while (len--) {
//		rt_kprintf("i2c_write: writing byte (%p)=0x%02x\n",
//		      buffer, *buffer);

		if (len == 0)
			msg.condition = I2C_COND_STOP;
		else
			msg.condition = I2C_COND_NORMAL;

		msg.acknack   = I2C_ACKNAK_WAITACK;
		msg.direction = I2C_WRITE;
		msg.data      = *(buffer++);

		if (i2c_transfer(base, &msg, sda_glitch_nofix))
			return -1;
	}

	i2c_reset(base, sda_glitch_nofix);

	return 0;
}

static struct spacemit_i2c *i2c_base = (struct spacemit_i2c *)0xc0886200;

static inline void mmio_write_32(rt_ubase_t addr, rt_uint32_t val)
{
	*(volatile rt_uint32_t *)addr = val;
}
static inline rt_uint32_t mmio_read_32(rt_ubase_t addr)
{
	return *(volatile rt_uint32_t *)addr;
}

/*
 * i2c_read: - Read multiple bytes from an i2c device
 *
 * The higher level routines take into account that this function is only
 * called with len < page length of the device (see configuration file)
 *
 * @chip:      address of the chip which is to be read
 * @addr:      i2c data address within the chip
 * @alen:      length of the i2c data address (1..2 bytes)
 * @buffer:    where to write the data
 * @len:       how much byte do we want to read
 * @return:    0 in case of success
 */
int spacemit_i2c_read(rt_uint8_t chip, rt_uint32_t addr, int alen, rt_uint8_t *buffer, int len)
{
	rt_uint8_t addr_bytes[4];

	addr_bytes[0] = (addr >> 0) & 0xFF;
	addr_bytes[1] = (addr >> 8) & 0xFF;
	addr_bytes[2] = (addr >> 16) & 0xFF;
	addr_bytes[3] = (addr >> 24) & 0xFF;

	return __i2c_read(i2c_base, chip, addr_bytes, alen, buffer, len, RT_FALSE);
}

/*
 * spacemit_i2c_write: -  Write multiple bytes to an i2c device
 *
 * The higher level routines take into account that this function is only
 * called with len < page length of the device (see configuration file)
 *
 * @chip:	address of the chip which is to be written
 * @addr:	i2c data address within the chip
 * @alen:	length of the i2c data address (1..2 bytes)
 * @buffer:	where to find the data to be written
 * @len:	how much byte do we want to read
 * @return:	0 in case of success
 */
int spacemit_i2c_write(rt_uint8_t chip, rt_uint32_t addr, int alen, rt_uint8_t *buffer, int len)
{
	rt_uint8_t addr_bytes[4];

	addr_bytes[0] = (addr >> 0) & 0xFF;
	addr_bytes[1] = (addr >> 8) & 0xFF;
	addr_bytes[2] = (addr >> 16) & 0xFF;
	addr_bytes[3] = (addr >> 24) & 0xFF;

	return __i2c_write(i2c_base, chip, addr_bytes, alen, buffer, len, RT_FALSE);
}
