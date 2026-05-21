/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>
#include <clint.h>
#include <spacemit_sdk_soc.h>

void rt_hw_us_delay(rt_uint32_t us)
{
	rt_uint64_t _start;
	rt_uint64_t _delte;

	_start = SysTimer_GetLoadValue();

	_delte = (rt_uint64_t)us * (SOC_TIMER_FREQ) / 1000000;

	while ((SysTimer_GetLoadValue() - _start) < _delte)
		asm volatile ("nop");
}

/**
 * This function will initial smart-evb board.
 */
void rt_hw_board_init(void)
{
#ifdef RT_USING_HEAP
    rt_system_heap_init((void *)RT_HEAP_START, (void *)RT_HEAP_END);
#endif

    /* uart must be initialize here */
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif

#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif
}

/*@}*/
