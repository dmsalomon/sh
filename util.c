/** \file util.c
 *
 * Provides some useful utility functions
 * for error handling.
 */

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

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

/*
 * fork and die on failure
 */
pid_t dfork()
{
	pid_t pid = fork();
	if (pid == -1)
		die("fork error:");
	return pid;
}

/*
 * die if memory was not alloc'd
 */
inline void alloc_die(void *p)
{
	if (!p)
		die("alloc_die():");
}
