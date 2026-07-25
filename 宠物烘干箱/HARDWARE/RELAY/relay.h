#ifndef __RELAY_H
#define __RELAY_H

#include "stm32f10x.h"

#define RELAY_GPIO_PORT    GPIOB
#define RELAY_GPIO_PIN     GPIO_Pin_11
#define RELAY_GPIO_CLK     RCC_APB2Periph_GPIOB

void Relay_Init(void);
void Relay_On(void);
void Relay_Off(void);
void Relay_Toggle(void);
#define Relay_GetStatus() ((GPIOB->ODR & RELAY_GPIO_PIN) ? 0 : 1)

#endif
