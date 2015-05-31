/*
 * olimex_p1114.c
 *
 * Olimex LPC_P1114 board support package.
 *
 * (c) 2015 Lixco Microsystems <lix@paulian.net>
 *
 * Created on: 30 May 2015 (LNP)
 */

#include <stdio.h>
#include "lpc_types.h"
#include "chip.h"
#include "olimex_p1114.h"

/* IOCON pin definitions for pin multiplexing */
typedef struct
{
	uint32_t pin :8; /* Pin number */
	uint32_t modefunc :24; /* Function and mode */
} pinmux_t;

/* System oscillator rate and clock rate on the CLKIN pin */
const uint32_t OscRateIn = HSE_VALUE;
const uint32_t ExtRateIn = 0;

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
static void UART_Init(void);

/**
 * @brief	Public functions.
 */

/**
 * @brief	This function initializes the system clocks and the
 * @brief	controller's pins and is called prior main;
 */
void SystemInit(void)
{
	/* Setup system clocking and muxing */
	SystemSetupClocking();
	SystemSetupMuxing();
}

/**
 * @brief	Setup and initialize the board's hardware.
 */
void Board_Init(void)
{
	/* Initialize GPIO */
	Chip_GPIO_Init(LPC_GPIO);

	/* Initialize LEDs */
	LED_Init();

	/* Initialize the UART */
	UART_Init();
}

/**
 * @brief	Sends a character on the UART.
 * @param	ch: the character to be sent.
 */
void UARTPutChar(char ch)
{
	Chip_UART_SendBlocking(LPC_USART, &ch, 1);
}

/**
 * @brief	Gets a characters from the UART.
 * @return	The character from the UART if any is received, EOF otherwise.
 */
int UARTGetChar(void)
{
	uint8_t data;

	if (Chip_UART_Read(LPC_USART, &data, 1) == 1)
		return (int) data;

	return EOF;
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

	/* Power-up main oscillator */
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_SYSOSC_PD);

	/* Wait 200us for OSC to be stabilized, no status
	 indication, dummy wait. */
	for (i = 0; i < 0x100; i++)
		;

	/* Set system PLL input to main oscillator */
	Chip_Clock_SetSystemPLLSource(SYSCTL_PLLCLKSRC_MAINOSC);

	/* Power down PLL to change the PLL divider ratio */
	Chip_SYSCTL_PowerDown(SYSCTL_POWERDOWN_SYSPLL_PD);

	/* Setup PLL for main oscillator rate (FCLKIN = 12MHz) * 4 = 48MHz
	 MSEL = 3 (this is pre-decremented), PSEL = 1 (for P = 2)
	 FCLKOUT = FCLKIN * (MSEL + 1) = 12MHz * 4 = 48MHz
	 FCCO = FCLKOUT * 2 * P = 48MHz * 2 * 2 = 192MHz (within FCCO range) */
	Chip_Clock_SetupSystemPLL(3, 1);

	/* Powerup system PLL */
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_SYSPLL_PD);

	/* Wait for PLL to lock */
	while (!Chip_Clock_IsSystemPLLLocked())
		;

	/* Set system clock divider to 1 */
	Chip_Clock_SetSysClockDiv(1);

	/* Setup FLASH access to 3 clocks */
	Chip_FMC_SetFLASHAccess(FLASHTIM_50MHZ_CPU);

	/* Set main clock source to the system PLL. This will drive 48MHz
	 for the main clock and 48MHz for the system clock */
	Chip_Clock_SetMainClockSource(SYSCTL_MAINCLKSRC_PLLOUT);
}

/**
 * @brief	Setup the system pin multiplexing.
 */
static void SystemSetupMuxing(void)
{
	int i;

	/* Enable IOCON clock */
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
	/* Set ports PIO3_0 to PIO3_5 and PIO2_6 and PIO2_7 as outputs */
	Chip_GPIO_SetPortDIROutput(LPC_GPIO, GPIO_PORT_3, 0x3F);
	Chip_GPIO_SetPortDIROutput(LPC_GPIO, GPIO_PORT_2, 0xC0);
}

/**
 * @brief	Initialize the UART.
 */
static void UART_Init(void)
{
	/* We assume that the Rx/Tx pins are already set at startup */
	/* Setup UART for 115.2K8N1 */
	Chip_UART_Init(LPC_USART);
	Chip_UART_SetBaud(LPC_USART, 115200);
	Chip_UART_ConfigData(LPC_USART, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(LPC_USART, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(LPC_USART);
}

