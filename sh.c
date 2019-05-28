/* \file sh.c
 *
 * A simple shell
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "cmd.h"
#include "error.h"
#include "eval.h"
#include "input.h"
#include "lexer.h"
#include "mem.h"
#include "parser.h"
#include "redir.h"
#include "trap.h"
#include "var.h"

int rootpid;

int main(int argc, char **argv)
{
	int exception;
	struct cmd *cmd;
	struct stackmark mark;
	struct jmploc jmploc;

	pushstackmark(&mark);
	rootpid = getpid();

	if ((exception = setjmp(jmploc.loc))) {
		/* reset the shell */
		unwindredir();
		unwindloops();
		popallfiles();
		yytoken = TNL;
		popstackmark(&mark);
		if (exception == EXINT)
			fputc('\n', stderr);
		FORCEINTON;
	} else {
		signal_init();
		initvar();
	}
	handler = &jmploc;

	pushstackmark(&mark);

	/* repl */
	for (; (cmd = parseline()) != CEOF; popstackmark(&mark)) {
		if (!cmd) continue;
		eval(cmd);
	}
}

