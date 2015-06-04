/*
 * @brief FreeRTOS Blinky example
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2012
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#include "olimex_p1114.h"
#include "FreeRTOS.h"
#include "task.h"


/* Sets up system hardware */
static void prvSetupHardware(void)
{
	SystemCoreClockUpdate();
	Board_Init();
}

/* LED0 toggle thread */
static void vLEDTask0(void *pvParameters)
{
	(void) pvParameters;

	while (1)
	{
		LED_Toggle(LED0);
		LED_Set(LED3, !LED_Test(LED0));
		vTaskDelay(configTICK_RATE_HZ / 2);
	}
}

/* LED1 toggle thread */
static void vLEDTask1(void *pvParameters)
{
	(void) pvParameters;

	while (1)
	{
		LED_Toggle(LED1);
		LED_Set(LED4, !LED_Test(LED1));
		vTaskDelay(configTICK_RATE_HZ);
	}
}

/* LED2 toggle thread */
static void vLEDTask2(void *pvParameters)
{
	(void) pvParameters;

	while (1)
	{
		LED_Toggle(LED2);
		LED_Set(LED5, !LED_Test(LED2));
		vTaskDelay(configTICK_RATE_HZ * 2);
	}
}

/* LED6/7 toggle thread */
static void vLEDTask3(void *pvParameters)
{
	(void) pvParameters;

	while (1)
	{
		LED_Toggle(LED6);
		LED_Set(LED7, !LED_Test(LED6));
		vTaskDelay(configTICK_RATE_HZ / 2);
	}
}

/**
 * @brief	main entry point.
 */
int main(void)
{
	prvSetupHardware();

	/* LED1 toggle thread */
	xTaskCreate(vLEDTask1, "vTaskLed1",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(xTaskHandle *) NULL);

	/* LED2 toggle thread */
	xTaskCreate(vLEDTask2, "vTaskLed2",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(xTaskHandle *) NULL);

	/* LED0 toggle thread */
	xTaskCreate(vLEDTask0, "vTaskLed0",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(xTaskHandle *) NULL);

	/* LED6 toggle thread */
	xTaskCreate(vLEDTask3, "vTaskLed3",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(xTaskHandle *) NULL);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Should never land here */
	for (;;)
		;
}
