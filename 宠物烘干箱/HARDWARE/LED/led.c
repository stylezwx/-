#include "led.h"
#include "delay.h"

void LED_Init(void)
{
	// 开启 GPIOC 时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	// PC13 配置为普通推挽输出
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = LED_GPIO_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(LED_GPIO_PROT, &GPIO_InitStructure);
	GPIO_ResetBits(LED_GPIO_PROT, LED_GPIO_PIN);
}

void LED_Toggle(void)
{
	GPIO_WriteBit(LED_GPIO_PROT, LED_GPIO_PIN, (BitAction)((1 - GPIO_ReadOutputDataBit(LED_GPIO_PROT, LED_GPIO_PIN))));
}

void LED_On(void)
{
	GPIO_WriteBit(LED_GPIO_PROT, LED_GPIO_PIN, (BitAction)0);
}

void LED_Off(void)
{
	GPIO_WriteBit(LED_GPIO_PROT, LED_GPIO_PIN, (BitAction)1);
}

void LED_Twinkle(void)
{
	LED_On();
	delay_ms(10);
	LED_Off();
}
