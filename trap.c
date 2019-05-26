
#include <signal.h>

#include "error.h"
#include "output.h"
#include "trap.h"

void signal_init(void)
{
	struct sigaction sa;

	sa.sa_flags = 0;
	sigfillset(&(sa.sa_mask));

	sa.sa_handler = onsig;
	if (sigaction(SIGINT, &sa, 0))
		die("sigaction: SIGINT:");

	sa.sa_handler = SIG_IGN;
	if (sigaction(SIGQUIT, &sa, 0))
		die("sigaction: SIGQUIT:");
	if (sigaction(SIGTSTP, &sa, 0))
		die("sigaction: SIGTSTP:");
}

void onsig(int sig)
{
	onint();
}
