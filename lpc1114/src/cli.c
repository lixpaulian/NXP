/*
 * cli.c
 *
 * Created on: 15 Mar 2011 (LNP)
 * Ported to the LPC1114 platform on: 4 Jun 2015 (LNP)
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

/*
 * This file implements a simple CLI (command line interface). Add/remove commands
 * as you wish, and as long as there is memory available.
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "cli.h"


/* CLI task defines */
#define BS 8					/* backspace */
#define TAB 9					/* tab */
#define CTRL_C 3				/* ctrl-C */
#define CTRL_X 0x18				/* ctrl-X */
#define ESC 0x1B				/* escape */

#define CIN_CANCEL -2			/* user abort */
#define CIN_UP_ARROW -3			/* up arrow */
#define CIN_DOWN_ARROW -4		/* down arrow */

#define CLI_HIST_PUT 1
#define CLI_HIST_NEXT 2
#define CLI_HIST_PREV 3

#define MAX_PARAMS 10			/* max number of parameters on the command line */

#define CLI_BUFF 64
#define STATS_BUFFER_SIZE 500
#define NR_RECORDS 10

/* local structures & co. */
struct cliHistory
{
	char	data[CLI_BUFF];
	unsigned int checksum;
};

/* CLI history storage area */
struct cliHistory history[NR_RECORDS];
extern volatile uint32_t uptime;
uint8_t	g_echo;
uint8_t g_errType;
extern int free_heap;

/* function prototypes */
static int cmdParser(char *prompt);
static int cliHist(int cmd, char *record);
static int help(int argc, char *argv[]);
static int getver(int argc, char *argv[]);
static int rtosStats(int argc, char *argv[]);
static int myExit (int argc, char *argv[]);
static int reboot(int argc, char *argv[]);
static int set_echo(int argc, char *argv[]);
static int getStrg(char *buffer, char *prompt, int history);
static int dump(int argc, char *argv[]);

/* CLI basic commands table */
const cmds_t clicmds[] =
		/*	CMD, function, help string */
{
		{ "ver", getver, "Show version and other system parameters" },
		{ "echo", set_echo, "Set/unset echo" },
		{ "sys", rtosStats, "Show FreeRTOS statistics" },
		{ "dump", dump, "Dump a memory zone" },
		{ "exit", myExit, "Exit monitor" },
		{ "reboot", reboot, "Reboot the system" },
		{ "help", help, "Show this help panel; for individual command help, use <command> -h" },
		{ NULL, NULL, 0 }
};


/**
 * @brief	Show the firmware and hardware version.
 * @param	argc: arguments count.
 * @param	argv: arguments list.
 * @return	always SUCCESS.
 */
int getver(int argc, char *argv[])
{
	(void) argc; (void) argv;

#ifdef DEBUG
	printf("%s firmware, ver. %s (debug)\r\n", PLATFORMNAME, VERSION);
#else
	printf("%s firmware, ver. %s\r\n", PLATFORMNAME, VERSION);
#endif
	printf("Build on %s\r\n", DATE);
	printf("Hardware %s rev. %s\r\n", HW_MODEL, HW_VERSION);
	printf("Core clock %ld MHz\r\n", SystemCoreClock / 1000000);
	printf("%s\r\n", COPYRIGHT);
	return SUCCESS;
}

/**
 * @brief	System statistics.
 * @param	argc: arguments count.
 * @param	argv: arguments list.
 * @return	always SUCCESS.
 */
static int rtosStats(int argc, char *argv[])
{
	char *statsBuffer;
	int	upt, mins;

	if (argc == 0) /* no parameters, return current status values */
	{
		if ((statsBuffer = pvPortMalloc(STATS_BUFFER_SIZE)))
		{
			upt = uptime / 60;		/* don't need the seconds */
			mins = upt % 60;
			upt /= 60;
			printf("up %d days, %d:%d\r\n", upt / 24, upt % 24, mins);
			printf("Heap: %d bytes free\r\n\n", free_heap);
			vTaskGetRunTimeStats(statsBuffer);
			strcat(statsBuffer, "\n");
			printf("%-16s%-16s%% Time\r\n", "Task", "Abs Time");
			printf("%s", statsBuffer);
			vTaskList(statsBuffer);
			printf("Task\t\tState\tPrio.\tStack\tID\r\n");
			printf("%s", statsBuffer);
			vPortFree(statsBuffer);
		}
	}
	else
	{
		if (argc > 1 || !strcmp(argv[0], "-h"))
			printf("Usage: sys\r\n");
	}
	return SUCCESS;
}

/**
 * @brief	Echo command: enable/disable echo.
 * @param	argc: arguments count.
 * @param	argv: arguments list.
 * @return	SUCCESS if parameters are OK, ERROR otherwise.
 */
static int set_echo(int argc, char *argv[])
{
	if (argc == 0)	/* no parameters, return current end of line character */
		printf("Echo is %s\r\n", g_echo ? "on" : "off");
	else
	{ 				/* set/unset */
		if (argc > 1 || !strcmp(argv[0], "-h"))
			printf("Usage: echo {on|off}\r\n");
		else
		{
			if (!strcmp(argv[0], "on"))
				g_echo = TRUE;
			else if (!strcmp(argv[0], "off"))
				g_echo = FALSE;
			else
			{
				g_errType = INVALID_PARAM;
				return ERROR;
			}
		}
	}
	return SUCCESS;
}

/**
 * @brief	Exit command (quits the CLI).
 * @param	argc: arguments count.
 * @param	argv: arguments list.
 * @return	ERROR and g_errType contains EXITCOMMAND error number.
 */
static int myExit(int argc, char *argv[])
{
	(void) argc; (void) argv;

	printf("Exiting...\r\n");
	vTaskDelay(10);				/* wait to finish the text */

	g_errType = EXITCOMMAND;
	return ERROR;
}

/**
 * @brief	Reboot command.
 * @param	argc: arguments count.
 * @param	argv: arguments list.
 * @return	if rebooted, will not return, SUCCESS otherwise.
 */
static int reboot(int argc, char *argv[])
{
	int	c;

	if (argc > 1 || ((argc == 1) && !strcmp(argv[0], "-h")))
	{
		printf("Usage: reboot\r\n");
		return SUCCESS;
	}
	printf("Are you sure? (y/n) ");
	c = getchar();
	printf("%c\r\n", c);
	if (c == 'y')
	{
		printf("System will now restart\r\n");
		vTaskDelay(1000);
		NVIC_SystemReset();
	}
	return SUCCESS;
}

/**
 * @brief	Memory dump command: dumps a zone of memory to the console.
 * @param	argc: arguments count.
 * @param	argv: arguments list.
 * @return	SUCCESS if the parameters are OK, ERROR otherwise.
 */
static int dump(int argc, char *argv[])
{
	int count, i, b_size;
	unsigned char *start, *tmp;
	unsigned char ch;
	unsigned int address;

	if (argc == 0 || !strcmp(argv[0], "-h"))
	{
		printf("Usage: dump start [size]\r\n");
		return SUCCESS;
	}

	if (sscanf(argv[0], "%x", &address))
	{
		start = (unsigned char *) address;

		b_size = 0x100;	/* set a default if no second parameter */
		if (argc == 2)
			sscanf(argv[1], "%x", &b_size);

		while (b_size > 0)
		{
			if ((b_size - 16) < 0)
				count = b_size;
			else
				count = 16;
			printf("%06X  ", (unsigned int) start);

			tmp = start;
			for (i = 0; i < count; i++)	/* hex dump */
				printf("%02X ", *start++);

			printf("  ");
			for (i = 0; i < count; i++)	/* ascii dump */
			{
				ch = *tmp++;
				if (isprint(ch))
					putchar(ch);
				else
					putchar('.');
			}
			printf("\r\n");
			b_size -=  16;
		}
		return SUCCESS;
	}
	g_errType = INVALID_PARAM;
	return ERROR;
}

/**
 * @brief	Command to print the help.
 * @param	argc: arguments count.
 * @param	argv: arguments list.
 * @return	always SUCCESS.
 */
static int help(int argc, char *argv[])
{
	(void) argc; (void) argv;
	const cmds_t *pcmd;

	for (pcmd = clicmds; pcmd->name; pcmd++)
		printf("  %-8s%s\r\n", pcmd->name, pcmd->help_string);
	return SUCCESS;
}

/**
 * @brief	Manage the CLI history memory. The CLI history structures are
 * 			initialized at the first CLI_HIST_PUT command.
 * @param	cmd: comamnds, can be: CLI_HIST_PUT, CLI_HIST_NEXT, CLI_HIST_PREV.
 * @param	record: pointer on a buffer to receive from, or store to the history
 * 			a command.
 * @return	SUCCESS if a valid record is found i.e. a record was successfully
 * 			stored or ERROR otherwise.
 */
static int cliHist(int cmd, char *record)
{
	static int counter = 0;	/* current read position on the array */
	static int load = 0;	/* current save position on the array */
	int i, saved;
	char *cpBuffer;
	uint16_t checksum;

	switch (cmd)
	{
	case CLI_HIST_PUT:
		if (!strlen(record))
			break; 					/* empty buffer, don't store */
									/* else, store buffer */
		memcpy(history[load].data, record, CLI_BUFF);
		for (i = 0, cpBuffer = history[load].data, checksum = 0;
				i < CLI_BUFF; i++)
			checksum += *cpBuffer++;
		history[load].checksum = checksum;
		if (++load >= NR_RECORDS)
			load = 0;				/* overflow, reset counter */
		counter = load;
		break;

	case CLI_HIST_PREV:
	case CLI_HIST_NEXT:
		saved = counter;			/* save counter */
		if (cmd == CLI_HIST_NEXT)	/* go forwards to the newest command */
		{
			if (++counter >= NR_RECORDS)
				counter = 0;
		}
		else
		{
			/* go backwards to the oldest command */
			if (--counter < 0)
				counter = NR_RECORDS - 1;
		}
		if (!strlen(history[counter].data) || counter == load)
		{							/* empty slot */
			counter = saved;		/* restore retrieve counter */
			break;
		}
		for (i = 0, cpBuffer = history[counter].data, checksum = 0;
				i < CLI_BUFF; i++)
			checksum += *cpBuffer++;
		if (checksum != history[counter].checksum)
			return ERROR; 			/* invalid record */
		memcpy(record, history[counter].data, CLI_BUFF);
		break;

	default:
		break;
	}
	return SUCCESS;
}

/**
 * @brief	Basic serial input/output routine: get a char with echo.
 * @return	a char, it blocks.
 */
static int getEcho(void)
{
	int c;
	char command = FALSE;

	for(;;)	/* wait for an input character */
	{
		switch (c = getchar())
		{
		case ESC:
			command = TRUE;
			continue;

		case '[':
			if (command == TRUE)
			{
				command = c;
				continue;
			}
			else if (g_echo)
				putchar(c);	/* echo the character back */
			break;

		case 'A':
			if (command == '[')
			{
				command = FALSE;
				c = CIN_UP_ARROW;
			}
			else if (g_echo)
				putchar(c);	/* echo the character back */
			break;

		case 'B':
			if (command == '[')
			{
				command = FALSE;
				c = CIN_DOWN_ARROW;
			}
			else if (g_echo)
				putchar(c);	/* echo the character back */
			break;

		case 'C':
			if (command == '[')
			{
				command = FALSE;	/* right arrow, not handled */
				break;
			}
			else if (g_echo)
				putchar(c);	/* echo the character back */
			break;

		case 'D':
			if (command == '[')
			{
				command = FALSE;	/* left arrow, not handled */
				break;
			}
			else if (g_echo)
				putchar(c);	/* echo the character back */
			break;

		default:
			if (g_echo)
				putchar(c);
		}
		break;	/* break-out the loop and return to caller */
	}
	return c;
}

/**
 * @brief	Get a whole string  end-of-line terminated, with echo.
 * @param	buffer: pointer on a buffer where to return the input string.
 * @param	prompt: prompt string.
 * @param	history: if TRUE, the string will be stored in the history.
 * @return	number of characters returned in the buffer.
 */
static int getStrg(char *buffer, char *prompt, int history)
{
	int i, c, overflow = FALSE;
	char *p;

	i = 0;
	p = buffer; 					/* save buffer start */
	memset(buffer, 0, CLI_BUFF);	/* zero the buffer, just for good practice... */

	while ((c = getEcho()))
	{
		if (c == EOF)				/* unlikely to happen, kept for extensibility */
			return EOF;

		if (c == '\r' || c == '\n')
			break;

		switch (c)
		{
		case CTRL_C:				/* cancel */
			return CIN_CANCEL;

		case CIN_UP_ARROW:
		case CIN_DOWN_ARROW:
			if (prompt != NULL)
			{
				printf("\r%c%s%s", ESC, "[K", prompt);
				buffer = p;			/* restore buffer start */
				if (cliHist(c == CIN_UP_ARROW ? CLI_HIST_PREV : CLI_HIST_NEXT,
						buffer) == ERROR)
				{
					i = 0;
					break;
				}
				i = strlen(buffer);
				printf("%s", buffer);
				buffer += i;
			}
			break;

		case BS:					/* backspace received */
			if (i > 0)
			{
				putchar(' ');
				putchar(BS);
				*buffer-- = '\0';
				i--;
			}
			else
				printf("%c%s", ESC, "[C");
			break;

		default:
			*buffer++ = (char) c;
			if (i++ >= CLI_BUFF)
			{
				buffer--;			/* if buffer overflow, wait */
				overflow = TRUE;
			}
		}
		if (overflow)
			break;
	}

	if (g_echo)
	{
		if (c == '\r') 				/* complete the newline sequence */
			printf("\n");			/* depending on what the host sent us */
		else if (c == '\n')
			printf("\r");
	}

	*buffer = '\0';					/* insert terminator */
	if (history)
		cliHist(CLI_HIST_PUT, p);	/* store in CLI history */

	return i;
}

/**
 * @brief	Command parsing function.
 * @param	prompt: prompt string.
 * @return	SUCCESS on normal exit (command EXIT).
 */
static int cmdParser(char *prompt)
{
	char buff[CLI_BUFF + 1], *pbuff;
	const cmds_t *pcmd;
	int i, result;
	char *argv[MAX_PARAMS]; /* pointers on parameters */
	int argc;				/* parameter counter */

	for (;;)
	{
		i = getStrg(buff, prompt, TRUE); /* get a string from user */

		if (i >= CLI_BUFF) 	/* CLI buffer overflow? */
		{
			printf("\rERROR 2\r\n%s", prompt);
			continue;
		}
		if (i == 0)			/* empty line? */
		{
			printf("\r\n%s", prompt);
			continue;		/* yes, nothing to parse, go back for next command */
		}
		pbuff = buff;
		while (*pbuff != ' ' && *pbuff != '\0')
			pbuff++;
		*pbuff++ = '\0'; 	/* insert terminator */

		result = ERROR;		/* initialize result just in case of failure */
		g_errType = CMD_NOT_FOUND;

		/* lookup in the commands table */
		for (pcmd = clicmds; pcmd->name; pcmd++)
		{
			if (!strcmp(buff, pcmd->name))
			{
				/* found valid command, parse parameters, if any */
				for (argc = 0; argc < MAX_PARAMS; argc++)
				{
					if (!*pbuff)
						break; 			/* end of line reached */
					if (*pbuff == '\"')
					{
						pbuff++; 		/* suck out the " */
						argv[argc] = pbuff;
						while (*pbuff != '\"' && *pbuff != '\0')
							pbuff++;
						*pbuff++ = '\0';
						if (*pbuff)		/* if not end of line... */
							pbuff++;	/* suck out the trailing space too */
					}
					else
					{
						argv[argc] = pbuff;
						while (*pbuff != ' ' && *pbuff != '\0')
							pbuff++;
						*pbuff++ = '\0';
					}
				}
				result = (*pcmd->func)(argc, &argv[0]); /* execute command */
				break;
			}
		}

		if (result == ERROR)
		{
			if (g_errType == EXITCOMMAND) /* exit command, must leave now */
				break;
			printf("ERROR %d\r\n%s", g_errType, prompt);
		}
		else
			printf("%s", prompt);
	}
	return SUCCESS;
}

/**
 * @brief	The serial console entry point.
 * @return	Success if properly exited.
 */
int theConsole()
{
	g_echo = TRUE;	/* set echo */

	printf("Type \"help\" for the list of available commands\r\n: ");
	return cmdParser(": ");
}


