/** \file util.h
 */

#ifndef UTIL_H
#define UTIL_H

#include <stdarg.h>

void vperrorf(const char *fmt, va_list);
void vpreperrorf(const char *pre, const char *fmt, va_list);
void perrorf(const char *fmt, ...);
void sdie(int status, const char *fmt, ...) __attribute__((noreturn));
void flushall(void);

#ifdef DEBUG
#define STRINGIFY(x) #x
#define STR(x) STRINGIFY(x)
#define DEBUGF(fmt, ...) perrorf("DEBUG:" __FILE__ ":" STR(__LINE__) ": " fmt, ##__VA_ARGS__)
#else
#define DEBUGF(fmt, ...)
#endif

#define die(...) sdie(2, __VA_ARGS__)

#endif
