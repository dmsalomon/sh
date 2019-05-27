
#include <signal.h>
#include <string.h>

#include "error.h"
#include "output.h"
#include "trap.h"

void signal_init(void)
{
	struct sigaction act;

	act.sa_flags = 0;
	sigfillset(&act.sa_mask);

	act.sa_handler = onsig;
	if (sigaction(SIGINT, &act, 0))
		die("sigaction: SIGINT:");

	act.sa_handler = SIG_IGN;
	int signals[] = {
		SIGQUIT, SIGTSTP, SIGTERM, SIGTTOU, SIGTTIN, 0,
	};

	for (int *p = signals; *p; p++)
		if (sigaction(*p, &act, 0))
			die("sigaction: %s:", strsignal(*p));
}

void onsig(int sig)
{
	if (sig == SIGINT) {
		if (!suppressint)
			onint();
		intpending = 1;
	}
}
