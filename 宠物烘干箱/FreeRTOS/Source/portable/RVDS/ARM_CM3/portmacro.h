#ifndef PORTMACRO_H
#define PORTMACRO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define portCHAR        char
#define portFLOAT       float
#define portDOUBLE      double
#define portLONG        long
#define portSHORT       short
#define portSTACK_TYPE  uint32_t
#define portBASE_TYPE   long

typedef portSTACK_TYPE StackType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;

#if( configUSE_16_BIT_TICKS == 1 )
    typedef uint16_t TickType_t;
    #define portMAX_DELAY ( TickType_t ) 0xffff
#else
    typedef uint32_t TickType_t;
    #define portMAX_DELAY ( TickType_t ) 0xffffffffUL
#endif

#define portTICK_PERIOD_MS            ( ( TickType_t ) 1000 / configTICK_RATE_HZ )

#define portBYTE_ALIGNMENT            8

#define portSTACK_GROWTH              ( -1 )

#define portTASK_FUNCTION_PROTO( vFunction, pvParameters ) void vFunction( void *pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters ) void vFunction( void *pvParameters )

#define portENTER_CRITICAL()          vPortEnterCritical()
#define portEXIT_CRITICAL()           vPortExitCritical()
#define portDISABLE_INTERRUPTS()      __asm volatile( "cpsid i" ::: "memory" )
#define portENABLE_INTERRUPTS()       __asm volatile( "cpsie i" ::: "memory" )

#define portYIELD()                   __asm volatile( "svc 0" )

#define portNOP()                     __asm volatile( "nop" )

#define portEND_SWITCHING_ISR( xSwitchRequired ) if( xSwitchRequired != pdFALSE ) portYIELD()

#define portRESTORE_CONTEXT()         __asm volatile( "ldmfd sp!, {r4-r11, pc}" )
#define portSAVE_CONTEXT()            __asm volatile( "stmfd sp!, {r4-r11}" )

#ifdef __cplusplus
}
#endif

#endif /* PORTMACRO_H */
