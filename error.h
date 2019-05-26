
#ifndef ERROR_H
#define ERROR_H

#include <setjmp.h>

struct jmploc {
	jmp_buf loc;
};

extern struct jmploc *handler;
extern int exception;

#define EXINT 0

void exraise(void) __attribute__((noreturn));
void onint(void) __attribute__((noreturn));

#endif
