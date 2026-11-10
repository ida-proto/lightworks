#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "board.h"

#define configUSE_PREEMPTION 1
#define configUSE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0
#define configCPU_CLOCK_HZ (BOARD_CPU_HZ)
#define configTICK_RATE_HZ 1000u
#define configMAX_PRIORITIES 5
#define configMINIMAL_STACK_SIZE 256u
#define configMAX_TASK_NAME_LEN 16
#define configUSE_16_BIT_TICKS 0
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#define configSUPPORT_STATIC_ALLOCATION 1
#define configSUPPORT_DYNAMIC_ALLOCATION 0
#define configUSE_MUTEXES 0
#define configUSE_TIMERS 0
#define configUSE_QUEUE_SETS 0
#define configUSE_NEWLIB_REENTRANT 0
#define configCHECK_FOR_STACK_OVERFLOW 2
#define configCHECK_HANDLER_INSTALLATION 1

/* Four implemented priority bits. Logical 5 is the syscall ceiling:
 * an ISR at numeric priority 5 or lower (higher number) may call FromISR.
 * SPI1 is placed at 6. */
#define configPRIO_BITS 4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY 15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
#define configKERNEL_INTERRUPT_PRIORITY (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

#define configASSERT(x)                                       \
  do {                                                        \
    if ((x) == 0) {                                           \
      __asm volatile("cpsid i");                              \
      for (;;) {                                              \
      }                                                       \
    }                                                         \
  } while (0)

#define INCLUDE_vTaskSuspend 1
#define INCLUDE_vTaskDelay 1
#define INCLUDE_xTaskDelayUntil 1

#endif /* FREERTOS_CONFIG_H */
