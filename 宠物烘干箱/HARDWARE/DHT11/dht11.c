#include "dht11.h"
#include "delay.h"

void DHT11_Rst(u8 sensor)
{
	if (sensor == 1)
	{
		DHT11_Mode(1, OUT);
		GPIO_ResetBits(DHT11_1_GPIO_PORT, DHT11_1_GPIO_PIN);
		delay_ms(20);
		GPIO_SetBits(DHT11_1_GPIO_PORT, DHT11_1_GPIO_PIN);
		delay_us(13);
	}
	else
	{
		DHT11_Mode(2, OUT);
		GPIO_ResetBits(DHT11_2_GPIO_PORT, DHT11_2_GPIO_PIN);
		delay_ms(20);
		GPIO_SetBits(DHT11_2_GPIO_PORT, DHT11_2_GPIO_PIN);
		delay_us(13);
	}
}

u8 DHT11_Check(u8 sensor)
{
	u8 retry = 0;

	if (sensor == 1)
	{
		DHT11_Mode(1, IN);
		while (GPIO_ReadInputDataBit(DHT11_1_GPIO_PORT, DHT11_1_GPIO_PIN) && retry < 100)
		{
			retry++;
			delay_us(1);
		}
		if (retry >= 100)
		{
			return 1;
		}

		retry = 0;
		while (!GPIO_ReadInputDataBit(DHT11_1_GPIO_PORT, DHT11_1_GPIO_PIN) && retry < 100)
		{
			retry++;
			delay_us(1);
		}
		if (retry >= 100)
		{
			return 1;
		}
	}
	else
	{
		DHT11_Mode(2, IN);
		while (GPIO_ReadInputDataBit(DHT11_2_GPIO_PORT, DHT11_2_GPIO_PIN) && retry < 100)
		{
			retry++;
			delay_us(1);
		}
		if (retry >= 100)
		{
			return 1;
		}

		retry = 0;
		while (!GPIO_ReadInputDataBit(DHT11_2_GPIO_PORT, DHT11_2_GPIO_PIN) && retry < 100)
		{
			retry++;
			delay_us(1);
		}
		if (retry >= 100)
		{
			return 1;
		}
	}

	return 0;
}

u8 DHT11_Read_Bit(u8 sensor)
{
	u8 retry = 0;

	if (sensor == 1)
	{
		while (GPIO_ReadInputDataBit(DHT11_1_GPIO_PORT, DHT11_1_GPIO_PIN) && retry < 100)
		{
			retry++;
			delay_us(1);
		}

		retry = 0;
		while (!GPIO_ReadInputDataBit(DHT11_1_GPIO_PORT, DHT11_1_GPIO_PIN) && retry < 100)
		{
			retry++;
			delay_us(1);
		}

		delay_us(40);
		return GPIO_ReadInputDataBit(DHT11_1_GPIO_PORT, DHT11_1_GPIO_PIN) ? 1 : 0;
	}
	else
	{
		while (GPIO_ReadInputDataBit(DHT11_2_GPIO_PORT, DHT11_2_GPIO_PIN) && retry < 100)
		{
			retry++;
			delay_us(1);
		}

		retry = 0;
		while (!GPIO_ReadInputDataBit(DHT11_2_GPIO_PORT, DHT11_2_GPIO_PIN) && retry < 100)
		{
			retry++;
			delay_us(1);
		}

		delay_us(40);
		return GPIO_ReadInputDataBit(DHT11_2_GPIO_PORT, DHT11_2_GPIO_PIN) ? 1 : 0;
	}
}

u8 DHT11_Read_Byte(u8 sensor)
{
	u8 i;
	u8 dat = 0;

	for (i = 0; i < 8; i++)
	{
		dat <<= 1;
		dat |= DHT11_Read_Bit(sensor);
	}

	return dat;
}

u8 DHT11_Read_Data(u8 sensor, u8 *temp, u8 *humi)
{
	u8 buf[5];
	u8 i;

	DHT11_Rst(sensor);
	if (DHT11_Check(sensor) != 0)
	{
		return 1;
	}

	for (i = 0; i < 5; i++)
	{
		buf[i] = DHT11_Read_Byte(sensor);
	}

	if ((u8)(buf[0] + buf[1] + buf[2] + buf[3]) != buf[4])
	{
		return 1;
	}

	*humi = buf[0];
	*temp = buf[2];
	return 0;
}

u8 DHT11_Init(u8 sensor)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	if (sensor == 1)
	{
		RCC_APB2PeriphClockCmd(DHT11_1_GPIO_CLK, ENABLE);
		GPIO_InitStructure.GPIO_Pin = DHT11_1_GPIO_PIN;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(DHT11_1_GPIO_PORT, &GPIO_InitStructure);
		GPIO_SetBits(DHT11_1_GPIO_PORT, DHT11_1_GPIO_PIN);
	}
	else
	{
		RCC_APB2PeriphClockCmd(DHT11_2_GPIO_CLK, ENABLE);
		GPIO_InitStructure.GPIO_Pin = DHT11_2_GPIO_PIN;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(DHT11_2_GPIO_PORT, &GPIO_InitStructure);
		GPIO_SetBits(DHT11_2_GPIO_PORT, DHT11_2_GPIO_PIN);
	}

	DHT11_Rst(sensor);
	return DHT11_Check(sensor);
}

void DHT11_Mode(u8 sensor, u8 mode)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	if (sensor == 1)
	{
		GPIO_InitStructure.GPIO_Pin = DHT11_1_GPIO_PIN;
		if (mode)
		{
			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		}
		else
		{
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		}
		GPIO_Init(DHT11_1_GPIO_PORT, &GPIO_InitStructure);
	}
	else
	{
		GPIO_InitStructure.GPIO_Pin = DHT11_2_GPIO_PIN;
		if (mode)
		{
			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		}
		else
		{
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		}
		GPIO_Init(DHT11_2_GPIO_PORT, &GPIO_InitStructure);
	}
}
