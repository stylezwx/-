#include "buzzer.h"

void Buzzer_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(BUZZER_GPIO_CLK, ENABLE);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = BUZZER_GPIO_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(BUZZER_GPIO_PROT, &GPIO_InitStructure);
	
	GPIO_ResetBits(BUZZER_GPIO_PROT, BUZZER_GPIO_PIN);
}

void Buzzer_On(void)
{
	GPIO_SetBits(BUZZER_GPIO_PROT, BUZZER_GPIO_PIN);
}

void Buzzer_Off(void)
{
	GPIO_ResetBits(BUZZER_GPIO_PROT, BUZZER_GPIO_PIN);
}

void Buzzer_Toggle(void)
{
	if(GPIO_ReadOutputDataBit(BUZZER_GPIO_PROT, BUZZER_GPIO_PIN))
		GPIO_ResetBits(BUZZER_GPIO_PROT, BUZZER_GPIO_PIN);
	else
		GPIO_SetBits(BUZZER_GPIO_PROT, BUZZER_GPIO_PIN);
}

void Buzzer_Beep(uint16_t time_ms)
{
	Buzzer_On();
	delay_ms(time_ms);
	Buzzer_Off();
}
