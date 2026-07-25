/***************STM32F103C8T6**********************
 * 文件名  ：usart1.c
 * 描述    ：将printf函数重定向到USART1。
 * 硬件连接：------------------------
 *          | PA9  - USART1(Tx)      |
 *          | PA10 - USART1(Rx)      |
 *           ------------------------
 * 库版本  ：ST3.0.0  *

********************LIGEN*************************/

#include "usart.h"
#include <stdarg.h>
#include <string.h>

#define USART1_RX_BUFFER_SIZE 20

static volatile USART1_Command g_usart1_cmd = USART1_CMD_NONE;
static volatile u8 g_usart1_pwm_duty = 0;
static volatile u8 g_usart1_rx_len = 0;
static volatile u8 g_usart1_line_ready = 0;
static volatile char g_usart1_rx_buf[USART1_RX_BUFFER_SIZE];

static void USART1_ResetBuffer(void)
{
	g_usart1_rx_len = 0;
	g_usart1_line_ready = 0;
	g_usart1_rx_buf[0] = '\0';
}

static u8 USART1_TryParsePwmDuty(const char *text, u8 *duty)
{
	u16 value = 0;
	u8 digit_count = 0;

	if(strncmp(text, "pwm=", 4) != 0)
	{
		return 0;
	}

	text += 4;
	while(*text != '\0')
	{
		if(*text < '0' || *text > '9')
		{
			return 0;
		}

		value = (u16)(value * 10 + (u16)(*text - '0'));
		if(value > 100)
		{
			return 0;
		}

		digit_count++;
		text++;
	}

	if(digit_count == 0)
	{
		return 0;
	}

	*duty = (u8)value;
	return 1;
}

static USART1_Command USART1_TryParseCommand(void)
{
	u8 pwm_duty;

	if(strcmp((const char *)g_usart1_rx_buf, "dog_small") == 0)
	{
		return USART1_CMD_DOG_SMALL;
	}
	else if(strcmp((const char *)g_usart1_rx_buf, "dog_medium") == 0 || strcmp((const char *)g_usart1_rx_buf, "dog") == 0)
	{
		return USART1_CMD_DOG_MEDIUM;
	}
	else if(strcmp((const char *)g_usart1_rx_buf, "dog_large") == 0)
	{
		return USART1_CMD_DOG_LARGE;
	}
	else if(strcmp((const char *)g_usart1_rx_buf, "cat_small") == 0)
	{
		return USART1_CMD_CAT_SMALL;
	}
	else if(strcmp((const char *)g_usart1_rx_buf, "cat_medium") == 0 || strcmp((const char *)g_usart1_rx_buf, "cat") == 0)
	{
		return USART1_CMD_CAT_MEDIUM;
	}
	else if(strcmp((const char *)g_usart1_rx_buf, "cat_large") == 0)
	{
		return USART1_CMD_CAT_LARGE;
	}
	else if(strcmp((const char *)g_usart1_rx_buf, "key_up") == 0 || strcmp((const char *)g_usart1_rx_buf, "up") == 0)
	{
		return USART1_CMD_KEY_UP;
	}
	else if(strcmp((const char *)g_usart1_rx_buf, "key_down") == 0 || strcmp((const char *)g_usart1_rx_buf, "down") == 0)
	{
		return USART1_CMD_KEY_DOWN;
	}
	else if(strcmp((const char *)g_usart1_rx_buf, "temp_up") == 0 || strcmp((const char *)g_usart1_rx_buf, "tempup") == 0)
	{
		return USART1_CMD_TEMP_UP;
	}
	else if(strcmp((const char *)g_usart1_rx_buf, "temp_down") == 0 || strcmp((const char *)g_usart1_rx_buf, "tempdown") == 0)
	{
		return USART1_CMD_TEMP_DOWN;
	}
	else if(strcmp((const char *)g_usart1_rx_buf, "pet_on") == 0)
	{
		return USART1_CMD_PET_MONITOR_ON;
	}
	else if(strcmp((const char *)g_usart1_rx_buf, "pet_off") == 0)
	{
		return USART1_CMD_PET_MONITOR_OFF;
	}
	else if(USART1_TryParsePwmDuty((const char *)g_usart1_rx_buf, &pwm_duty))
	{
		g_usart1_pwm_duty = pwm_duty;
		return USART1_CMD_SET_PWM;
	}

	return USART1_CMD_NONE;
}


void USART1_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* 使能 USART1 时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE); 

	/* USART1 使用IO端口配置 */    
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //复用推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);    
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;	//浮空输入
  GPIO_Init(GPIOA, &GPIO_InitStructure);   //初始化GPIOA
	  
	/* USART1 工作模式配置 */
	USART_InitStructure.USART_BaudRate = 115200;	//波特率设置：115200
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;	//数据位数设置：8位
	USART_InitStructure.USART_StopBits = USART_StopBits_1; 	//停止位设置：1位
	USART_InitStructure.USART_Parity = USART_Parity_No ;  //是否奇偶校验：无
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	//硬件流控制模式设置：没有使能
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;//接收与发送都使能
	USART_Init(USART1, &USART_InitStructure);  //初始化USART1

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART1, ENABLE);// USART1使能
}


 /* 描述  ：重定向c库函数printf到USART1*/ 
int fputc(int ch, FILE *f)
{
/* 将Printf内容发往串口 */
  USART_SendData(USART1, (unsigned char) ch);
  while (!(USART1->SR & USART_FLAG_TXE));
 
  return (ch);
}

USART1_Command USART1_GetCommand(void)
{
	if(g_usart1_line_ready)
	{
		g_usart1_cmd = USART1_TryParseCommand();
		USART1_ResetBuffer();
	}

	USART1_Command cmd = g_usart1_cmd;
	g_usart1_cmd = USART1_CMD_NONE;
	return cmd;
}

u8 USART1_GetPwmDuty(void)
{
	return g_usart1_pwm_duty;
}

void USART1_IRQHandler(void)
{
	char data;

	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		data = (char)USART_ReceiveData(USART1);

		if(g_usart1_line_ready)
		{
			USART_ClearITPendingBit(USART1, USART_IT_RXNE);
			return;
		}

		if(data == '\r' || data == '\n')
		{
			if(g_usart1_rx_len > 0)
			{
				g_usart1_rx_buf[g_usart1_rx_len] = '\0';
				g_usart1_line_ready = 1;
			}
		}
		else
		{
			if(g_usart1_rx_len < USART1_RX_BUFFER_SIZE - 1)
			{
				g_usart1_rx_buf[g_usart1_rx_len++] = data;
				g_usart1_rx_buf[g_usart1_rx_len] = '\0';
			}
			else
			{
				USART1_ResetBuffer();
			}
		}

		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	}
}




