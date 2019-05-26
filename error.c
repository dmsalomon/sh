
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>

#include "error.h"
#include "eval.h"
#include "output.h"
#include "trap.h"

struct jmploc *handler;

void exraise(int e)
{
	if (!handler)
		abort();

	longjmp(handler->loc, e);
}

void onint(void)
{
	sigclearmask();
	exitstatus = SIGINT + 128;
	exraise(EXINT);
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
