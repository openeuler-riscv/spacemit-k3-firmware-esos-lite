#ifndef RT_CONFIG_H__
#define RT_CONFIG_H__

/* Automatically generated file; DO NOT EDIT. */
/* RT-Thread Configuration */

/* RT-Thread Kernel */

#define RT_NAME_MAX 8
#define RT_ALIGN_SIZE 4
#define RT_THREAD_PRIORITY_32
#define RT_THREAD_PRIORITY_MAX 32
#define RT_TICK_PER_SECOND 100
#define IDLE_THREAD_STACK_SIZE 2048

/* kservice optimization */

#define RT_KSERVICE_USING_TINY_SIZE

/* Inter-Thread communication */

#define RT_USING_SEMAPHORE

/* Memory Management */

#define RT_USING_MEMHEAP
#define RT_USING_NOHEAP

/* Kernel Device Object */

#define RT_USING_DEVICE
#define RT_USING_CONSOLE
#define RT_CONSOLEBUF_SIZE 128
#define RT_CONSOLE_DEVICE_NAME "uart0"
#define RT_VER_NUM 0x40004
#define ARCH_CPU_64BIT
#define RT_USING_CACHE
#define ARCH_RISCV
#define ARCH_RISCV64

/* RT-Thread Components */

#define RT_USING_COMPONENTS_INIT
#define RT_USING_USER_MAIN
#define RT_MAIN_THREAD_STACK_SIZE 4096
#define RT_MAIN_THREAD_PRIORITY 10

/* Command shell */


/* Device Drivers */

#define RT_USING_DEVICE_IPC
#define RT_PIPE_BUFSZ 512
#define RT_USING_SERIAL
#define RT_USING_SERIAL_V1
#define RT_SERIAL_USING_DMA
#define RT_SERIAL_RB_BUFSZ 64
#define RT_USING_PM

/* Using USB */


/* Hardware Drivers Config */

/* Onboard Peripheral Drivers */

/* On-chip Peripheral Drivers */

#define BSP_USING_PM

/* Board extended module Drivers */

/* Platform Config */

#define SOC_SPACEMIT_K3

#endif
