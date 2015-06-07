/*
 * syscalls.c
 *
 * Created on: Jun 4, 2015 (LNP)
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

/* A minimum implementation of the stubs required by newlib */

#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include "FreeRTOS.h"
#include "task.h"
#include "olimex_p1114.h"

/* initial heap size, it will be properly set after the first malloc() */
int free_heap = 8192 - 1024;


caddr_t _sbrk(int incr)
{
	extern char _pvHeapStart; /* see linker definitions */
	extern char _vStackTop;

	static char* current_heap_end;
	char* current_block_address;

	if (current_heap_end == 0)
		current_heap_end = &_pvHeapStart;

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
	free_heap = &_vStackTop - current_heap_end;

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

int __attribute__((weak))
_lseek(int file __attribute__((unused)), int ptr __attribute__((unused)),
    int dir __attribute__((unused)))
{
  errno = ENOSYS;
  return -1;
}

int __attribute__((weak))
_fstat(int fildes __attribute__((unused)),
    struct stat* st __attribute__((unused)))
{
  errno = ENOSYS;
  return -1;
}

int __attribute__((weak))
_close(int fildes __attribute__((unused)))
{
  errno = ENOSYS;
  return -1;
}

int __attribute__((weak))
_isatty(int file __attribute__((unused)))
{
  errno = ENOSYS;
  return 0;
}

