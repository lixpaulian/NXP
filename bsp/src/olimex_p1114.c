/*
 * olimex_p1114.c
 *
 * Olimex LPC_P1114 board support package.
 *
 * Created on: 30 May 2015 (LNP)
 *
 * Copyright (c) 2011-2015 Lixco Microsystems <lix@paulian.net>
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

#include <stdio.h>
#include "lpc_types.h"
#include "chip.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "olimex_p1114.h"

/* CLI UART baud rate */
#define BAUD_RATE 115200

/* serial queues size */
#define TX_QUEUE_SIZE 64
#define RX_QUEUE_SIZE 64

/* USART transmit and receive queues */
QueueHandle_t txQueue;
QueueHandle_t rxQueue;

volatile int g_Uart_Error = 0;

/* system oscillator rate and clock rate on the CLKIN pin */
const uint32_t OscRateIn = HSE_VALUE;
const uint32_t ExtRateIn = 0;

/* IOCON pin definitions for pin multiplexing */
typedef struct
{
	uint32_t pin :8; /* Pin number */
	uint32_t modefunc :24; /* Function and mode */
} pinmux_t;

/**
 *  @brief	Pin multiplexing table, only items that need changing from their default
 *  @brief	pin state are in this table. Modify/add entries depending on your application.
 */
static const pinmux_t pinmuxing[] =
{
{ (uint32_t) IOCON_PIO0_2, (IOCON_FUNC1 | IOCON_MODE_INACT) }, /* PIO0_2 used for SSEL */
{ (uint32_t) IOCON_PIO0_4, (IOCON_FUNC1 | IOCON_SFI2C_EN) }, /* PIO0_4 used for SCL */
{ (uint32_t) IOCON_PIO0_5, (IOCON_FUNC1 | IOCON_SFI2C_EN) }, /* PIO0_5 used for SDA */
{ (uint32_t) IOCON_PIO0_8, (IOCON_FUNC1 | IOCON_MODE_INACT) }, /* PIO0_8 used for MISO */
{ (uint32_t) IOCON_PIO0_9, (IOCON_FUNC1 | IOCON_MODE_INACT) }, /* PIO0_9 used for MOSI */
{ (uint32_t) IOCON_PIO1_6, (IOCON_FUNC1 | IOCON_MODE_INACT) }, /* PIO1_6 used for RXD */
{ (uint32_t) IOCON_PIO1_7, (IOCON_FUNC1 | IOCON_MODE_INACT) }, /* PIO1_7 used for TXD */
{ (uint32_t) IOCON_PIO2_11, (IOCON_FUNC1 | IOCON_MODE_INACT) }, /* PIO0_6 used for SCK */
};

/* Forward declarations */
static void SystemSetupClocking(void);
static void SystemSetupMuxing(void);
static void LED_Init(void);
static void UART_Init(int baudrate);

/**
 * @brief	Public functions.
 */

/**
 * @brief	This function initializes the system clocks and the
 * @brief	controller's pins and is called prior main;
 */
void SystemInit(void)
{
	/* setup system clocking and pin functions */
	SystemSetupClocking();
	SystemSetupMuxing();
}

/**
 * @brief	Setup and initialize the board's hardware.
 */
void Board_Init(void)
{
	/* initialize GPIO */
	Chip_GPIO_Init(LPC_GPIO);

	/* initialize LEDs */
	LED_Init();

	/* initialize UART */
	UART_Init(BAUD_RATE);
}

/**
 * @brief	Configure timer32_0 to count up every 100 us; this is used by
 * @brief	FreeRTOS statistics functions.
 */
void vMainConfigureTimerForRunTimeStats(void)
{
	/* initialize 32-bit timer0 clock */
	Chip_TIMER_Init(LPC_TIMER32_0);

	/* reset the timer terminal and prescale counts */
	Chip_TIMER_Reset(LPC_TIMER32_0);

	/* setup prescale value to result in a count every 100 us */
	Chip_TIMER_PrescaleSet(LPC_TIMER32_0, 5000);

	/* start timer */
	Chip_TIMER_Enable(LPC_TIMER32_0);
}

/**
 * @brief	Get the current value of the timer32_0.
 * @return	The current value of the timer32_0.
 */
uint32_t ulMainGetRunTimeCounterValue(void)
{
	return Chip_TIMER_ReadCount(LPC_TIMER32_0);
}

/**
 * @brief	Wait until a byte is available in the FIFO of the serial interface.
 * @param	timeout: maximum time to wait for a byte. If portMAX_DELAY
 * 			is specified, the function will block.
 * @retval the character received or: EOF (-1) if none (i.e. timeout),
 * 			UART_ERROR (-2) if an UART error occurred.
 */
int getCharSerial(int timeout)
{
	int value = 0;

	if ((xQueueReceive(rxQueue, &value, timeout) != pdPASS))
		value = EOF;
	if (g_Uart_Error)
	{
		value = UART_ERROR;
		g_Uart_Error = FALSE;
	}
	return value;
}

/**
 * @brief	Send a char through the serial interface if it's not busy.
 * @param	buff: pointer on a buffer containing the characters to be sent.
 * @param	len: number of characters to be sent.
 * @retval	Number of characters sent.
 */
int sendCharSerial(uint8_t *buff, int len)
{
	int	count = 0;

	if (len)
	{
		while (len--)
		{
			if (xQueueSendToBack(txQueue, buff, MS10_DELAY) == pdPASS)
			{
				buff++;
				count++;
			}
			else
				break;	/* queue full, exit */
		}
		if (count)
			Chip_UART_IntEnable(LPC_USART, UART_IER_THREINT);
	}
	return count;
}

/**
 * @brief	Test if a character is pending in the input stream.
 * @retval	TRUE if at least a character is pending, FALSE otherwise.
 */
int kbHit(void)
{
	unsigned portBASE_TYPE nrItems = 0;

	nrItems = uxQueueMessagesWaiting(rxQueue);
	return (nrItems != 0);
}

/**
 * @brief	Sets the state of a LED to on or off.
 * @param	LEDNumber: the number if the LED to be set.
 * @param	state: the new state of the LED (TRUE or FALSE).
 */
void LED_Set(uint8_t LEDNumber, bool state)
{
	Chip_GPIO_SetPinState(LPC_GPIO,
			LEDNumber < LED6 ? GPIO_PORT_3 : GPIO_PORT_2, LEDNumber, state);
}

/**
 * @brief	Returns the current state of a LED.
 * @param	LEDNumber: the number of the LED to return the status.
 * @return	True if the LED is on, FALSE otherwise.
 */
bool LED_Test(uint8_t LEDNumber)
{
	return Chip_GPIO_GetPinState(LPC_GPIO,
			LEDNumber < LED6 ? GPIO_PORT_3 : GPIO_PORT_2, LEDNumber);
}

/**
 * @brief	Toggle the state of a LED.
 * @param	LEDNumber: the number if the LED to be toggled.
 */
void LED_Toggle(uint8_t LEDNumber)
{
	Chip_GPIO_SetPinToggle(LPC_GPIO,
			LEDNumber < LED6 ? GPIO_PORT_3 : GPIO_PORT_2, LEDNumber);
}

/**
 * @brief	Static functions.
 */

/**
 * @brief	Setup the system clocks.
 */
static void SystemSetupClocking(void)
{
	volatile int i;

	/* power-up main oscillator */
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_SYSOSC_PD);

	/* wait 200us for OSC to stabilize, no status
	 indication, dummy wait. */
	for (i = 0; i < 0x100; i++)
		;

	/* set system PLL input to main oscillator */
	Chip_Clock_SetSystemPLLSource(SYSCTL_PLLCLKSRC_MAINOSC);

	/* power down PLL to change the PLL divider ratio */
	Chip_SYSCTL_PowerDown(SYSCTL_POWERDOWN_SYSPLL_PD);

	/* setup PLL for main oscillator rate (FCLKIN = 12MHz) * 4 = 48MHz
	 MSEL = 3 (this is pre-decremented), PSEL = 1 (for P = 2)
	 FCLKOUT = FCLKIN * (MSEL + 1) = 12MHz * 4 = 48MHz
	 FCCO = FCLKOUT * 2 * P = 48MHz * 2 * 2 = 192MHz (within FCCO range) */
	Chip_Clock_SetupSystemPLL(3, 1);

	/* power up the system PLL */
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_SYSPLL_PD);

	/* wait for PLL to lock */
	while (!Chip_Clock_IsSystemPLLLocked())
		;

	/* set system clock divider to 1 */
	Chip_Clock_SetSysClockDiv(1);

	/* setup FLASH access to 3 clocks */
	Chip_FMC_SetFLASHAccess(FLASHTIM_50MHZ_CPU);

	/* set main clock source to the system PLL; this will drive 48MHz
	 for the main clock and 48MHz for the system clock */
	Chip_Clock_SetMainClockSource(SYSCTL_MAINCLKSRC_PLLOUT);
}

/**
 * @brief	Setup the system pin multiplexing.
 */
static void SystemSetupMuxing(void)
{
	int i;

	/* enable IOCON clock */
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_IOCON);

	for (i = 0; i < (int) (sizeof(pinmuxing) / sizeof(pinmux_t)); i++)
	{
		Chip_IOCON_PinMuxSet(LPC_IOCON, (CHIP_IOCON_PIO_T) pinmuxing[i].pin,
				pinmuxing[i].modefunc);
	}
}

/**
 * @brief	Initialize the LED ports.
 */
static void LED_Init(void)
{
	/* set ports PIO3_0 to PIO3_5 and PIO2_6 and PIO2_7 as outputs */
	Chip_GPIO_SetPortDIROutput(LPC_GPIO, GPIO_PORT_3, 0x3F);
	Chip_GPIO_SetPortDIROutput(LPC_GPIO, GPIO_PORT_2, 0xC0);
}

/**
 * @brief	Initialize the UART.
 */
static void UART_Init(int baudrate)
{
	/* we assume that the Rx/Tx pins are already set at startup */
	/* setup UART for 115.2K, 8N1 */
	Chip_UART_Init(LPC_USART);
	Chip_UART_SetBaud(LPC_USART, baudrate);
	Chip_UART_ConfigData(LPC_USART, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(LPC_USART, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(LPC_USART);

	/* create queues */
	txQueue = xQueueCreate(TX_QUEUE_SIZE, sizeof(uint8_t));
	rxQueue = xQueueCreate(RX_QUEUE_SIZE, sizeof(uint8_t));

	/* enable receive data and line status interrupt */
	Chip_UART_IntEnable(LPC_USART, UART_IER_RBRINT);

	/* enable UART interrupt */
	NVIC_EnableIRQ(UART0_IRQn);
}

/**
 * @brief	Handle UART interrupt.
 */
void UART_IRQHandler(void)
{
	uint8_t ch;
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	/* handle transmit interrupt if enabled */
	if (((Chip_UART_ReadLineStatus(LPC_USART) & UART_LSR_THRE) != 0) &&
			xQueueReceiveFromISR(txQueue, &ch, &xHigherPriorityTaskWoken) == pdPASS)
	{
		Chip_UART_SendByte(LPC_USART, ch);
	}
	else
	/* disable transmit interrupt if the queue is empty */
		Chip_UART_IntDisable(LPC_USART, UART_IER_THREINT);

	/* handle receive interrupt */
	while ((Chip_UART_ReadLineStatus(LPC_USART) & UART_LSR_RDR) != 0)
	{
		ch = Chip_UART_ReadByte(LPC_USART);
		xQueueSendToBackFromISR(rxQueue, &ch, &xHigherPriorityTaskWoken);
	}
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}


