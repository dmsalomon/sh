
#ifndef ERROR_H
#define ERROR_H

#include <setjmp.h>

struct jmploc {
	jmp_buf loc;
};

extern struct jmploc *handler;
extern int exception;

/* must be non zero */
#define EXINT 1
#define EXERR 2

void exraise(int) __attribute__((noreturn));
void onint(void) __attribute__((noreturn));
void raiseerr(const char *, ...) __attribute__((noreturn));

#endif
