/* \file sh.c
 *
 * A simple shell
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "util.h"
#include "cmd.h"

int lstatus = 0;

static void signal_init();

int main(int argc, char **argv)
{
	static struct cmd *cmd;

	signal_init();

	/* repl */
	while (getcmd(&cmd)) {
		if (!cmd) continue;
		lstatus = runcmd(cmd);
	}
}

static void handler(int signum) { }

static void signal_init()
{
	struct sigaction sa;

	sa.sa_handler = handler;
	sa.sa_flags = SA_RESTART;
	sigfillset(&sa.sa_mask);

	if (sigaction(SIGINT, &sa, NULL))
		die("sigaction: SIGINT:");
	if (sigaction(SIGQUIT, &sa, NULL))
		die("sigaction: SIGQUIT:");
}

