#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"

#define KEY_UP_PORT      GPIOA
#define KEY_UP_PIN       GPIO_Pin_0
#define KEY_DOWN_PORT    GPIOA
#define KEY_DOWN_PIN     GPIO_Pin_1
#define KEY_TEMP_UP_PORT GPIOA
#define KEY_TEMP_UP_PIN  GPIO_Pin_2
#define KEY_TEMP_DOWN_PORT GPIOA
#define KEY_TEMP_DOWN_PIN  GPIO_Pin_3

typedef enum {
    KEY_NONE = 0,
    KEY_UP,
    KEY_DOWN,
    KEY_TEMP_UP,
    KEY_TEMP_DOWN
} Key_TypeDef;

void Key_Init(void);
Key_TypeDef Key_Scan(void);

#endif
