#ifndef __BUZZER_H
#define __BUZZER_H

#include "stm32f10x.h"
#include "delay.h"

#define BUZZER_GPIO_PROT    GPIOB
#define BUZZER_GPIO_PIN     GPIO_Pin_13
#define BUZZER_GPIO_CLK     RCC_APB2Periph_GPIOB

void Buzzer_Init(void);
void Buzzer_On(void);
void Buzzer_Off(void);
void Buzzer_Toggle(void);
void Buzzer_Beep(uint16_t time_ms);

#endif
