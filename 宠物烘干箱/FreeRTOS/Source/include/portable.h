#ifndef PORTABLE_H
#define PORTABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"

#define portINITIAL_XPSR            ( 0x01000000 )
#define portSTART_ADDRESS_MASK      ( ( StackType_t ) 0xfffffffe )

extern StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters );
extern BaseType_t xPortStartScheduler( void );
extern void vPortEndScheduler( void );
extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );

#ifdef __cplusplus
}
#endif

#endif /* PORTABLE_H */
