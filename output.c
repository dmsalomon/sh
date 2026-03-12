/** \file util.c
 *
 * Provides some useful utility functions
 * for error handling.
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "eval.h"
#include "input.h"
#include "options.h"
#include "output.h"

/*
 * Like perror but with formatting
 */
void vpreperrorf(const char *pre, const char *fmt, va_list ap) {
  const char *prefmt = commandname ? "%s: %d: %s: " : "%s: %d: ";
  fprintf(stderr, prefmt, arg0, plineno - 1, commandname);
  if (pre)
    fputs(pre, stderr);
  vfprintf(stderr, fmt, ap);

  if (fmt[0] && fmt[strlen(fmt) - 1] != ':') {
    fputc('\n', stderr);
  } else {
    fputc(' ', stderr);
    perror(NULL);
  }
}

void vperrorf(const char *fmt, va_list ap) {
  vpreperrorf(NULL, fmt, ap);
}

void perrorf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vperrorf(fmt, ap);
  va_end(ap);
}

/*
 * Taken from https://git.suckless.org/dmenu, util.c
 */
void sdie(int status, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vperrorf(fmt, ap);
  va_end(ap);
  _exit(status);
}

void flushall(void) {
  fflush(stdout);
  fflush(stderr);
}
