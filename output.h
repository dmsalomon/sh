/** \file util.h
 */

#ifndef UTIL_H
#define UTIL_H

#define PROGNAME "sh"

void perrorf(const char *fmt, ...);
void reportf(const char *fmt, ...);
void sdie(int status, const char *fmt, ...) __attribute__((noreturn));
pid_t dfork(void);
void alloc_die(void *);

#define die(...) sdie(1, __VA_ARGS__)

#endif
