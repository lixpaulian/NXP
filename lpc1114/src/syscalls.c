/*
 * syscalls.c
 *
 * (c) 2015 Lixco Microsystems <lix@paulian.net>
 *
 * Created on: Jun 4, 2015 (LNP)
 */


#include <sys/types.h>
#include <errno.h>
#include "FreeRTOS.h"
#include "task.h"
#include "olimex_p1114.h"


caddr_t _sbrk(int incr)
{
	extern char _pvHeapStart; /* see linker definition */
	extern char _vStackTop;

	static char* current_heap_end;
	char* current_block_address;

	if (current_heap_end == 0)
	{
		current_heap_end = &_pvHeapStart;
	}

	current_block_address = current_heap_end;

	/* align heap to word boundary */
	incr = (incr + 3) & (~3);
	if (current_heap_end + incr > &_vStackTop)
	{
		/* heap overflow */
		errno = ENOMEM;
		return (caddr_t) -1;
	}

	current_heap_end += incr;

	return (caddr_t) current_block_address;
}

int _write (int file, char *ptr, int len)
{
	int	n = 0;

	if (!len)
		return 0;

	switch(file)
	{
	case 1:		/* stdout */
	case 2:		/* stderr */
		n = sendCharSerial((uint8_t *) ptr, len);
		break;
	}
	return n;
}

int _read (int file, char *ptr, int len)
{
	int n = 0;

	if (!len)
		return 0;

	switch(file)
	{
	case 0:		/* stdin */
		if ((n = getCharSerial(portMAX_DELAY)) != EOF)
		{
			*ptr = (uint8_t) n;
			n = 1;
		}
		else
			n = 0;
		break;
	}
	return n;
}

