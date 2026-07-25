#ifndef __PWM_H
#define __PWM_H

#include "stm32f10x.h"

typedef enum {
    PWM_LEVEL_OFF = 0,
    PWM_LEVEL_1 = 1,
    PWM_LEVEL_2 = 2,
    PWM_LEVEL_3 = 3
} PWM_Level;

void PWM_Init(void);
void PWM_SetDuty(u16 duty);
u16 PWM_GetDuty(void);
void PWM_SetLevel(PWM_Level level);
PWM_Level PWM_GetLevel(void);

#endif
