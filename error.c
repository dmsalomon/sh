
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

#include "error.h"
#include "eval.h"
#include "output.h"
#include "trap.h"

struct jmploc *handler;
int suppressint;
volatile sig_atomic_t intpending;

void exraise(int e)
{
	if (!handler)
		abort();

	if (forked)
		_exit(exitstatus);

	INTOFF;

	longjmp(handler->loc, e);
}

void onint(void)
{
	intpending = 0;
	sigclearmask();
	exitstatus = SIGINT + 128;
	exraise(EXINT);
}

void raiseexc(int e, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vperrorf(fmt, ap);

	exraise(e);

	/* unreachable */
	va_end(ap);
}

void raiseerr(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vperrorf(fmt, ap);

	exitstatus = 2;
	exraise(EXERR);

	/* unreachable */
	va_end(ap);
}

void __inton(void)
{
	if (--suppressint == 0 && intpending)
		onint();
}
