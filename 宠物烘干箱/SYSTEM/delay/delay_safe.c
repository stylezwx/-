#include "stm32f10x.h"

#define DWT_CTRL_REG (*(volatile u32 *)0xE0001000UL)
#define DWT_CYCCNT_REG (*(volatile u32 *)0xE0001004UL)

static u32 cycles_per_us = 1U;
static volatile u32 system_millis = 0U;

void delay_init(u8 sysclk_mhz)
{
    cycles_per_us = sysclk_mhz;

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT_CYCCNT_REG = 0U;
    DWT_CTRL_REG |= 1U;

    if(SysTick_Config((u32)sysclk_mhz * 1000U) != 0U)
    {
        while(1)
        {
        }
    }
}

void delay_us(u32 microseconds)
{
    u32 start = DWT_CYCCNT_REG;
    u32 required_cycles = microseconds * cycles_per_us;

    while((u32)(DWT_CYCCNT_REG - start) < required_cycles)
    {
    }
}

void delay_ms(u16 milliseconds)
{
    while(milliseconds > 0U)
    {
        delay_us(1000U);
        milliseconds--;
    }
}

u32 delay_get_ms(void)
{
    return system_millis;
}

void delay_tick_inc(void)
{
    system_millis++;
}
