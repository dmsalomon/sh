/** \file util.h
 */

#ifndef UTIL_H
#define UTIL_H

#include <stdarg.h>

void vperrorf(const char *fmt, va_list);
void perrorf(const char *fmt, ...);
void sdie(int status, const char *fmt, ...) __attribute__((noreturn));
void flushall(void);

#ifdef DEBUG
#define DEBUGF(fmt, ...) perrorf("DEBUG: " fmt, __VA_ARGS__)
#else
#define DEBUGF(fmt, ...)
#endif


#define die(...) sdie(2, __VA_ARGS__)

#endif
