#ifndef FREERTOS_H
#define FREERTOS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOSConfig.h"

#ifndef configUSE_PREEMPTION
    #define configUSE_PREEMPTION 1
#endif

#ifndef configUSE_IDLE_HOOK
    #define configUSE_IDLE_HOOK 0
#endif

#ifndef configUSE_TICK_HOOK
    #define configUSE_TICK_HOOK 0
#endif

#ifndef configCPU_CLOCK_HZ
    #define configCPU_CLOCK_HZ ( ( unsigned long ) 72000000 )
#endif

#ifndef configTICK_RATE_HZ
    #define configTICK_RATE_HZ ( ( TickType_t ) 1000 )
#endif

#ifndef configMAX_PRIORITIES
    #define configMAX_PRIORITIES ( ( unsigned portBASE_TYPE ) 5 )
#endif

#ifndef configMINIMAL_STACK_SIZE
    #define configMINIMAL_STACK_SIZE ( ( unsigned short ) 128 )
#endif

#ifndef configTOTAL_HEAP_SIZE
    #define configTOTAL_HEAP_SIZE ( ( size_t ) ( 17 * 1024 ) )
#endif

#ifndef configMAX_TASK_NAME_LEN
    #define configMAX_TASK_NAME_LEN ( 16 )
#endif

#ifndef configUSE_TRACE_FACILITY
    #define configUSE_TRACE_FACILITY 0
#endif

#ifndef configUSE_STATS_FORMATTING_FUNCTIONS
    #define configUSE_STATS_FORMATTING_FUNCTIONS 0
#endif

#ifndef configUSE_16_BIT_TICKS
    #define configUSE_16_BIT_TICKS 0
#endif

#ifndef configIDLE_SHOULD_YIELD
    #define configIDLE_SHOULD_YIELD 1
#endif

#ifndef INCLUDE_vTaskPrioritySet
    #define INCLUDE_vTaskPrioritySet 1
#endif

#ifndef INCLUDE_uxTaskPriorityGet
    #define INCLUDE_uxTaskPriorityGet 1
#endif

#ifndef INCLUDE_vTaskDelete
    #define INCLUDE_vTaskDelete 1
#endif

#ifndef INCLUDE_vTaskCleanUpResources
    #define INCLUDE_vTaskCleanUpResources 0
#endif

#ifndef INCLUDE_vTaskSuspend
    #define INCLUDE_vTaskSuspend 1
#endif

#ifndef INCLUDE_vTaskDelayUntil
    #define INCLUDE_vTaskDelayUntil 1
#endif

#ifndef INCLUDE_vTaskDelay
    #define INCLUDE_vTaskDelay 1
#endif

#ifndef configUSE_MUTEXES
    #define configUSE_MUTEXES 1
#endif

#ifndef configUSE_RECURSIVE_MUTEXES
    #define configUSE_RECURSIVE_MUTEXES 1
#endif

#ifndef configUSE_COUNTING_SEMAPHORES
    #define configUSE_COUNTING_SEMAPHORES 1
#endif

#ifndef configQUEUE_REGISTRY_SIZE
    #define configQUEUE_REGISTRY_SIZE 0
#endif

#ifndef configUSE_QUEUE_SETS
    #define configUSE_QUEUE_SETS 0
#endif

#ifndef configUSE_TIME_SLICING
    #define configUSE_TIME_SLICING 1
#endif

#ifndef configUSE_NEWLIB_REENTRANT
    #define configUSE_NEWLIB_REENTRANT 0
#endif

#ifndef configENABLE_BACKWARD_COMPATIBILITY
    #define configENABLE_BACKWARD_COMPATIBILITY 1
#endif

#ifndef configSUPPORT_STATIC_ALLOCATION
    #define configSUPPORT_STATIC_ALLOCATION 0
#endif

#ifndef configSUPPORT_DYNAMIC_ALLOCATION
    #define configSUPPORT_DYNAMIC_ALLOCATION 1
#endif

#ifndef configKERNEL_INTERRUPT_PRIORITY
    #define configKERNEL_INTERRUPT_PRIORITY 255
#endif

#ifndef configMAX_SYSCALL_INTERRUPT_PRIORITY
    #define configMAX_SYSCALL_INTERRUPT_PRIORITY 191
#endif

#include "portable.h"

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_H */
