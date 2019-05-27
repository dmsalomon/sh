
#ifndef ERROR_H
#define ERROR_H

#include <setjmp.h>
#include <signal.h>

struct jmploc {
	jmp_buf loc;
};

extern struct jmploc *handler;
extern int exception;

extern int suppressint;
extern volatile sig_atomic_t intpending;

/* must be non zero */
#define EXINT 1
#define EXERR 2

#define barrier() ({ __asm__ __volatile__ ("": : :"memory"); })
#define INTOFF \
	({ \
		suppressint++; \
		barrier(); \
		0; \
	})
void __inton(void);
#define INTON __inton()
#define FORCEINTON \
	({ \
		barrier(); \
		suppressint = 0; \
		if (intpending) onint(); \
		0; \
	})

void exraise(int) __attribute__((noreturn));
void onint(void) __attribute__((noreturn));
void raiseexc(int, const char *, ...) __attribute__((noreturn));
void raiseerr(const char *, ...) __attribute__((noreturn));

#endif
