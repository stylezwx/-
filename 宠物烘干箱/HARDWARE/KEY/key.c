#include "key.h"
#include "delay.h"

void Key_Init(void)
{
    RCC->APB2ENR |= (1 << 2);

    GPIOA->CRL &= ~(0xFFFF << 0);
    GPIOA->CRL |= (0x8888 << 0);

    GPIOA->ODR |= (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3);
}

Key_TypeDef Key_Scan(void)
{
    static u8 key_up[4] = {1, 1, 1, 1};

    if(key_up[0] && !(GPIOA->IDR & (1 << 0)))
    {
        delay_ms(10);
        key_up[0] = 0;
        if(!(GPIOA->IDR & (1 << 0)))
            return KEY_UP;
    }
    else if(GPIOA->IDR & (1 << 0))
        key_up[0] = 1;

    if(key_up[1] && !(GPIOA->IDR & (1 << 1)))
    {
        delay_ms(10);
        key_up[1] = 0;
        if(!(GPIOA->IDR & (1 << 1)))
            return KEY_DOWN;
    }
    else if(GPIOA->IDR & (1 << 1))
        key_up[1] = 1;

    if(key_up[2] && !(GPIOA->IDR & (1 << 2)))
    {
        delay_ms(10);
        key_up[2] = 0;
        if(!(GPIOA->IDR & (1 << 2)))
            return KEY_TEMP_UP;
    }
    else if(GPIOA->IDR & (1 << 2))
        key_up[2] = 1;

    if(key_up[3] && !(GPIOA->IDR & (1 << 3)))
    {
        delay_ms(10);
        key_up[3] = 0;
        if(!(GPIOA->IDR & (1 << 3)))
            return KEY_TEMP_DOWN;
    }
    else if(GPIOA->IDR & (1 << 3))
        key_up[3] = 1;

    return KEY_NONE;
}
