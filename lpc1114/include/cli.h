/*
 * cli.h
 *
 * Created on: 15 Mar 2011 (LNP)
 * Ported to the LPC1114 platform on: 4 Jun 2015 (LNP)
 *
 * (c) 2011-2014 Lixco Microsystems <lix@paulian.net>
 */

#ifndef CLI_H_
#define CLI_H_

#define PLATFORMNAME "LPC1114"
#define VERSION "0.1"
#define DATE (__DATE__ " " __TIME__)
#define HW_MODEL "Olimex LPC-P1114"
#define HW_VERSION "A"
#define COPYRIGHT "(c) 2015 Lixco Microsystems"

/* CLI errors */
#define CMD_NOT_FOUND 1			/* the CLI could not locate the command */
#define LINE_TO_LONG 2			/* CLI buffer overflow */
#define INTERNAL 3				/* internal error */
#define PARAM_TOO_LONG 4		/* parameter is too long */
#define INVALID_PARAM 5			/* invalid parameter */
#define MALLOC_ERROR 8			/* error allocating memory (out of memory) */
#define EXITCOMMAND 99			/* user induced exit */

/* this structure defines an entry in the command table */
typedef struct
{
    char *name;
    int (*func) ();
    char *help_string;
} cmds_t;


int theConsole();

#endif /* CLI_H_ */
