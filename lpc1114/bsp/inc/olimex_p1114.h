/*
 * olimex_p1114.h
 *
 * Olimex LPC_P1114 board support package.
 *
 * (c) 2015 Lixco Microsystems <lix@paulian.net>
 *
 * Created on: 30 May 2015 (LNP)
 */

#ifndef __OLIMEX_P1114_H_
#define __OLIMEX_P1114_H_

#include "lpc_types.h"
#include <stdio.h>

enum leds_t
{
	LED0, LED1, LED2, LED3, LED4, LED5, LED6, LED7
};

enum gpio_ports_t
{
	GPIO_PORT_0, GPIO_PORT_1, GPIO_PORT_2, GPIO_PORT_3
};

void Board_Init(void);
void UARTPutChar(char ch);
int UARTGetChar(void);
void LED_Set(uint8_t LEDNumber, bool State);
bool LED_Test(uint8_t LEDNumber);
void LED_Toggle(uint8_t LEDNumber);

#endif /* __OLIMEX_P1114_H_ */
