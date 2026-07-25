#include "relay.h"

void Relay_Init(void)
{
    RCC->APB2ENR |= (1 << 3);

    GPIOB->CRH &= ~(0xF << 12);
    GPIOB->CRH |= (0x3 << 12);

    GPIOB->BSRR = RELAY_GPIO_PIN;
}

void Relay_On(void)
{
    GPIOB->BRR = RELAY_GPIO_PIN;
}

void Relay_Off(void)
{
    GPIOB->BSRR = RELAY_GPIO_PIN;
}

void Relay_Toggle(void)
{
    if(GPIOB->ODR & RELAY_GPIO_PIN)
    {
        GPIOB->BRR = RELAY_GPIO_PIN;
    }
    else
    {
        GPIOB->BSRR = RELAY_GPIO_PIN;
    }
}
