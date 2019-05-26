
#include <signal.h>
#include <stdlib.h>

#include "error.h"
#include "trap.h"

extern int exitstatus;

struct jmploc *handler;
int exception;

void exraise(void)
{
	if (!handler)
		abort();

	longjmp(handler->loc, 1);
}

void onint(void)
{
	sigclearmask();
	exitstatus = SIGINT + 128;
	exraise();
}


