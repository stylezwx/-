#include "pwm.h"

static PWM_Level current_level = PWM_LEVEL_OFF;
#define PWM_PERIOD_COUNTS 2880U

void PWM_Init(void)
{
    RCC->APB2ENR |= (1 << 2);
    RCC->APB1ENR |= (1 << 1);

    GPIOA->CRL &= ~(0xF << 24);
    GPIOA->CRL |= (0xB << 24);

    TIM3->PSC = 0;
    TIM3->ARR = PWM_PERIOD_COUNTS - 1;
    TIM3->CCR1 = 0;
    
    TIM3->CCMR1 &= ~(0x7 << 4);
    TIM3->CCMR1 |= (0x6 << 4);
    TIM3->CCMR1 |= (1 << 3);
    
    TIM3->CCER |= (1 << 0);
    
    TIM3->CR1 |= (1 << 7);
    TIM3->CR1 |= (1 << 0);
}

void PWM_SetDuty(u16 duty)
{
    if(duty > 100) duty = 100;
    TIM3->CCR1 = (PWM_PERIOD_COUNTS * duty) / 100;
}

u16 PWM_GetDuty(void)
{
    return (TIM3->CCR1 * 100U) / PWM_PERIOD_COUNTS;
}

void PWM_SetLevel(PWM_Level level)
{
    current_level = level;
    switch(level)
    {
        case PWM_LEVEL_OFF:
            PWM_SetDuty(0);
            break;
        case PWM_LEVEL_1:
            PWM_SetDuty(30);
            break;
        case PWM_LEVEL_2:
            PWM_SetDuty(60);
            break;
        case PWM_LEVEL_3:
            PWM_SetDuty(90);
            break;
        default:
            PWM_SetDuty(0);
            break;
    }
}

PWM_Level PWM_GetLevel(void)
{
    return current_level;
}
