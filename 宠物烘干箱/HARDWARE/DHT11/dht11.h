#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f10x.h"                  // Device header
#include "delay.h"

/*****************���絥Ƭ�����******************
											STM32
 * �ļ�			:	DHT11��ʪ�ȴ�����h�ļ�                   
 * �汾			: V1.0
 * ����			: 2024.8.4
 * MCU			:	STM32F103C8T6
 * �ӿ�			:	������	
 * IP�˺�		:	���絥Ƭ����ƣ�ͬBILIBILI|����|����|С����|CSDN|���ں�|��Ƶ�ŵȣ�
 * ����			:	����
 * ������		: �췽�����ӹ�����
 * ������Ƶ	:	https://www.bilibili.com/video/BV182421Z7by/?share_source=copy_web&vd_source=097fdeaf6b6ecfed8a9ff7119c32faf2
 * �ٷ���վ	:	www.yfcdz.cn

**********************BEGIN***********************/		


/***************�����Լ��������****************/
//DHT11���ź궨��
#define DHT11_1_GPIO_PORT  GPIOA
#define DHT11_1_GPIO_PIN   GPIO_Pin_11
#define DHT11_1_GPIO_CLK   RCC_APB2Periph_GPIOA
#define DHT11_2_GPIO_PORT  GPIOA
#define DHT11_2_GPIO_PIN   GPIO_Pin_12
#define DHT11_2_GPIO_CLK   RCC_APB2Periph_GPIOA
/*********************END**********************/

//���״̬����
#define OUT 1
#define IN  0

u8 DHT11_Init(u8 sensor);//��ʼ��DHT11, sensor: 1-DHT11_1, 2-DHT11_2
u8 DHT11_Read_Data(u8 sensor, u8 *temp,u8 *humi);//��ȡ��ʪ������
u8 DHT11_Read_Byte(u8 sensor);//��ȡһ���ֽڵ�����
u8 DHT11_Read_Bit(u8 sensor);//��ȡһλ������
void DHT11_Mode(u8 sensor, u8 mode);//DHT11�������ģʽ����
u8 DHT11_Check(u8 sensor);//���DHT11
void DHT11_Rst(u8 sensor);//��λDHT11   

#endif
