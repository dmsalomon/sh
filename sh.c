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
#include "output.h"
#include "parser.h"
#include "redir.h"
#include "sh.h"
#include "trap.h"
#include "var.h"

int rootpid;

static int repl(void);

int main(int argc, char **argv)
{
	int exception;
	struct stackmark mark;
	struct jmploc jmploc;

	INTOFF;
	if ((exception = setjmp(jmploc.loc))) {
		/* reset the shell */
		unwindredir();
		unwindloops();
		popallfiles();
		yytoken = TNL;
		popstackmark(&mark);
		if (exception == EXINT)
			fputc('\n', stderr);
	} else {
		pushstackmark(&mark);
		rootpid = getpid();
		initvar();
		signal_init();
	}
	handler = &jmploc;
	FORCEINTON;

	return repl();
}

static int repl(void)
{
	int status = 0;
	struct cmd *cmd;
	struct stackmark mark;

	pushstackmark(&mark);

	for (; (cmd = parseline()) != CEOF; popstackmark(&mark)) {
		if (!cmd) continue;
		status = eval(cmd);
	}

	return status;
}

int source_builtin(struct cexec *cmd)
{
	int status;

	if (cmd->argc < 2) {
		perrorf("%s: not enough arguments", cmd->argv[0]);
		return 1;
	}

	setinputfile(cmd->argv[1], INPUT_PUSH_FILE);
	status = repl();
	popfile();
	return status;
}
