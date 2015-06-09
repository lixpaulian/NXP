/*
 * olimex_p1114.h
 *
 * Olimex LPC_P1114 board support package.
 *
 * Created on: 30 May 2015 (LNP)
 *
 * (c) 2015 Lixco Microsystems <lix@paulian.net>
 *
 */

#ifndef __OLIMEX_P1114_H_
#define __OLIMEX_P1114_H_

#include "lpc_types.h"
#include <stdio.h>

/* defines for some typical delays */
#define ONE_SECOND_DELAY	(( portTickType) 1000 / portTICK_RATE_MS)
#define MS500_DELAY			((portTickType) 500 / portTICK_RATE_MS)
#define MS300_DELAY			((portTickType) 300 / portTICK_RATE_MS)
#define MS200_DELAY			((portTickType) 200 / portTICK_RATE_MS)
#define MS100_DELAY			((portTickType) 100 / portTICK_RATE_MS)
#define MS50_DELAY			((portTickType) 50 / portTICK_RATE_MS)
#define MS40_DELAY			((portTickType) 40 / portTICK_RATE_MS)
#define MS30_DELAY			((portTickType) 30 / portTICK_RATE_MS)
#define MS20_DELAY			((portTickType) 20 / portTICK_RATE_MS)
#define MS10_DELAY			((portTickType) 10 / portTICK_RATE_MS)
#define MS5_DELAY			((portTickType) 5 / portTICK_RATE_MS)
#define MS2_DELAY			((portTickType) 2 / portTICK_RATE_MS)
#define MS1_DELAY			((portTickType) 1 / portTICK_RATE_MS)

#define UART_ERROR (-2)

enum leds_t
{
	LED0, LED1, LED2, LED3, LED4, LED5, LED6, LED7
};

enum gpio_ports_t
{
	GPIO_PORT_0, GPIO_PORT_1, GPIO_PORT_2, GPIO_PORT_3
};

void Board_Init(void);
void vMainConfigureTimerForRunTimeStats(void);
uint32_t ulMainGetRunTimeCounterValue(void);
int getCharSerial(int timeout);
int sendCharSerial(uint8_t *buff, int len);
int kbHit(void);
void LED_Set(uint8_t LEDNumber, bool State);
bool LED_Test(uint8_t LEDNumber);
void LED_Toggle(uint8_t LEDNumber);

#endif /* __OLIMEX_P1114_H_ */
