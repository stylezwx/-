#include "FreeRTOS.h"
#include "task.h"

#if( configCPU_CLOCK_HZ == 0 )
    #error configCPU_CLOCK_HZ must be defined in FreeRTOSConfig.h
#endif

#define portNVIC_SYSTICK_CTRL_REG    ( ( volatile uint32_t *) 0xe000e010 )
#define portNVIC_SYSTICK_LOAD_REG    ( ( volatile uint32_t *) 0xe000e014 )
#define portNVIC_SYSPRI2_REG         ( ( volatile uint32_t *) 0xe000ed20 )
#define portNVIC_PENDSV_PRI          ( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 16 )
#define portNVIC_SYSTICK_PRI         ( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 24 )

static void prvPortStartFirstTask( void );
extern void xPortPendSVHandler( void );
extern void xPortSysTickHandler( void );

StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters )
{
    pxTopOfStack--;
    *pxTopOfStack = portINITIAL_XPSR;

    pxTopOfStack--;
    *pxTopOfStack = ( ( StackType_t ) pxCode ) & portSTART_ADDRESS_MASK;

    pxTopOfStack -= 5;
    *pxTopOfStack = 0;

    pxTopOfStack -= 8;
    *pxTopOfStack = ( StackType_t ) pvParameters;

    pxTopOfStack -= 8;

    return pxTopOfStack;
}

BaseType_t xPortStartScheduler( void )
{
    portNVIC_SYSPRI2_REG = portNVIC_PENDSV_PRI;
    portNVIC_SYSTICK_PRI = portNVIC_SYSTICK_PRI;

    portNVIC_SYSTICK_LOAD_REG = configCPU_CLOCK_HZ / configTICK_RATE_HZ;
    portNVIC_SYSTICK_CTRL_REG = ( 1UL << 0UL ) | ( 1UL << 1UL ) | ( 1UL << 2UL );

    prvPortStartFirstTask();

    return 0;
}

void vPortEndScheduler( void )
{
}

static void prvPortStartFirstTask( void )
{
    __asm volatile(
        "ldr r0, =0xe000ed08\n"
        "ldr r0, [r0]\n"
        "ldr r0, [r0]\n"
        "msr msp, r0\n"
        "cpsie i\n"
        "cpsie f\n"
        "dsb\n"
        "isb\n"
        "svc 0\n"
        "nop\n"
    );
}

void vPortEnterCritical( void )
{
    portDISABLE_INTERRUPTS();
}

void vPortExitCritical( void )
{
    portENABLE_INTERRUPTS();
}
