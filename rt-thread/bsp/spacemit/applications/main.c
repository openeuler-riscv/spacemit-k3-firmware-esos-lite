/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtservice.h>
#include <rtdevice.h>

int main(void)
{
	/* enter idle */
	rt_pm_release(RT_PM_DEFAULT_SLEEP_MODE);

	return 0;
}
