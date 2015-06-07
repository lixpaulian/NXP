/*
 * main.c
 *
 * Copyright (c) 2015 Lixco Microsystems <lix@paulian.net>
 *
 * Created on: May 30, 2015 (LNP)
 *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* This is a small platform build on an Olimex LPC-P1114 board; it can serve
 * as a starting point for other projects. The project is build using the
 * "gnuarmeclipse" plugins (http://gnuarmeclipse.livius.net).
 *
 * In this file we start two tasks: a LED blinking task and a serial CLI task
 * (command line interface). Add your own tasks depending on the application
 * and of course of the available memory. You can remove the CLI task, or some
 * or all of its commands and add other commands.
 */

#include <stdio.h>
#include "olimex_p1114.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cli.h"

/* uptime variable */
volatile uint32_t uptime = 0;


/**
 * @brief	This task increments the uptime and blinks LED6 and LED7 alternatively.
 * @param 	pvParameters: not used.
 */
static void vLEDTask(void *pvParameters)
{
	(void) pvParameters;

	for (;;)
	{
		LED_Toggle(LED6);
		LED_Set(LED7, !LED_Test(LED6));
		vTaskDelay(configTICK_RATE_HZ / 2);	/* half a second */
		if (LED_Test(LED6))
			uptime++;
	}
}

/**
 * @brief	The serial CLI task is started here.
 * @param	pvParameters: not used.
 */
static void cliTask(void *pvParameters)
{
	(void) pvParameters;

	/* disable output buffering */
	setvbuf(stdout, NULL, _IONBF, 0);

	/* launch the console */
	for (;;)
		theConsole();
}

/**
 * @brief	Start the system: main entry point.
 */
int main(void)
{
	SystemCoreClockUpdate();
	Board_Init();

	/* create the CLI task */
	xTaskCreate(cliTask, "cli",
			configMINIMAL_STACK_SIZE * 5, NULL, (tskIDLE_PRIORITY + 1UL),
			(xTaskHandle *) NULL);

	/* create the LEDs toggle task */
	xTaskCreate(vLEDTask, "blinkLEDs",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 2UL),
			(xTaskHandle *) NULL);

	/* start the scheduler */
	vTaskStartScheduler();

	/* should never land here */
	for (;;)
		;
}
