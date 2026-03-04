/** \file util.h
 */

#ifndef UTIL_H
#define UTIL_H

#include <stdarg.h>

void vperrorf(const char *fmt, va_list);
void perrorf(const char *fmt, ...);
void sdie(int status, const char *fmt, ...) __attribute__((noreturn));
void flushall(void);

#define die(...) sdie(2, __VA_ARGS__)

#endif
