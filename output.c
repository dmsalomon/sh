/** \file util.c
 *
 * Provides some useful utility functions
 * for error handling.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "input.h"
#include "output.h"

/*
 * Like perror but with formatting
 */
void vperrorf(const char *fmt, va_list ap)
{
	fprintf(stderr, "%s: %d: ", PROGNAME, plineno-1);
	vfprintf(stderr, fmt, ap);

	if (fmt[0] && fmt[strlen(fmt)-1] != ':') {
		fputc('\n', stderr);
	} else {
		fputc(' ', stderr);
		perror(NULL);
	}
}

void perrorf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vperrorf(fmt, ap);
	va_end(ap);
}

/*
 * Taken from https://git.suckless.org/dmenu, util.c
 */
void sdie(int status, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vperrorf(fmt, ap);
	va_end(ap);
	_exit(status);
}

void flushall(void)
{
	fflush(stdout);
	fflush(stderr);
}
