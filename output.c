/** \file util.c
 *
 * Provides some useful utility functions
 * for error handling.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "output.h"

void reportf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "%s: ", PROGNAME);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

/*
 * Like perror but with formatting
 */
void perrorf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "%s: ", PROGNAME);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt)-1] != ':') {
		fputc('\n', stderr);
	} else {
		fputc(' ', stderr);
		perror(NULL);
	}
}

/*
 * Taken from https://git.suckless.org/dmenu, util.c
 */
void sdie(int status, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "%s: ", PROGNAME);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt)-1] != ':') {
		fputc('\n', stderr);
	} else {
		fputc(' ', stderr);
		perror(NULL);
	}

	exit(status);
}
