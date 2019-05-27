/** \file cmd.c
 *
 * evaluate commands.
 */

#include <errno.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "builtin.h"
#include "cmd.h"
#include "error.h"
#include "eval.h"
#include "input.h"
#include "mem.h"
#include "output.h"
#include "redir.h"
#include "parser.h"
#include "trap.h"

int exitstatus;
int forked;

static pid_t dfork(void);
static int evalpipe(struct cmd *);
static int evalloop(struct cmd *);

int eval(struct cmd *c)
{
	struct cexec *ce;
	struct cif   *ci;
	struct cbinary *cb;
	struct credir *cr;
	struct cunary *cu;

	switch (c->type) {
	case CEXEC:
		ce = (struct cexec*)c;
		exitstatus = evalcmd(ce);
		break;

	case CREDIR:
		cr = (struct credir*)c;
		if (pushredirect(cr) >= 0) {
			exitstatus = eval(cr->cmd);
			popredirect();
		}
		break;

	case CPIPE:
		exitstatus = evalpipe(c);
		break;

	case CBANG:
		cu = (struct cunary*)c;
		exitstatus = !eval(cu->cmd);
		break;

	case CAND:
	case COR:
		cb = (struct cbinary*)c;

		if (eval(cb->left) ^ (cb->type == CAND))
			exitstatus = eval(cb->right);
		break;

	case CLIST:
		cb = (struct cbinary*)c;

		exitstatus = eval(cb->left);
		if (cb->right)
			exitstatus = eval(cb->right);
		break;

	case CIF:
		ci = (struct cif*)c;

		if (!eval(ci->cond))
			exitstatus = eval(ci->ifpart);
		else if (ci->elsepart)
			exitstatus = eval(ci->elsepart);

		break;

	case CWHILE:
	case CUNTIL:
		exitstatus = evalloop(c);
		break;

	case CBRC:
	case CSUB:
		/* for now subshells are fake */
		cu = (struct cunary*)c;
		exitstatus = eval(cu->cmd);
		break;

	default:
		perrorf("%d: not implemented", c->type);
	}

	return exitstatus;
}

/*
 * runs the command
 *
 * if its a builtin it runs the corresponding funcion
 * otherwise it forks and execs the program
 *
 * returns the exitstatus
 */
int evalcmd(struct cexec *cmd)
{
	int i;
	char **argv;
	struct arg *ap;

	argv = stalloc(sizeof(*argv)*(cmd->argc+1));
	for (i = 0, ap = cmd->args; i < cmd->argc; i++, ap = ap->next)
		argv[i] = ap->text;
	argv[i] = NULL;
	cmd->argv = argv;

	/* hack the $? for now */
	/* TODO support real variables */
	static char buf[32];
	snprintf(buf, 32, "%d", exitstatus);
	for (i =0; i <cmd->argc;i++)
		if (strcmp(cmd->argv[i], "$?") == 0)
			cmd->argv[i] = buf;

	builtin_func bt = get_builtin(cmd->argv[0]);

	if (bt)
		return (*bt)(cmd);
	else
		return runprog(cmd);
}

int evalstring(char *s)
{
	struct cmd *cmd;
	struct stackmark mark;
	int status;

	s = sstrdup(s);
	setinputstring(s);
	pushstackmark(&mark);

	status = 0;
	for (; (cmd = parseline()) != CEOF; popstackmark(&mark))
		if (cmd)
			status = eval(cmd);

	popfile();
	stfree(s);

	return status;
}

int eval_builtin(struct cexec *cmd)
{
	char *p;
	char *concat;
	char **ap;

	if (cmd->argc < 2) {
		return 0;
	} else if (cmd->argc == 2) {
		p = cmd->argv[1];
	} else {
		STARTSTACKSTR(concat);
		for (ap = cmd->argv + 1; *ap; ap++) {
			for (p = *ap; *p; p++)
				STPUTC(*p, concat);
			STPUTC(' ', concat);
		}
		STPUTC('\0', concat);
		p = ststrsave(concat);
	}
	return evalstring(p);
}

static int evalpipe(struct cmd *c)
{
	int pip[2], status;
	pid_t pl, pr;
	struct cbinary *cmd;

	cmd = (struct cbinary *)c;

	if ((pr = dfork()) == 0) {
		pipe(pip);
		if ((pl = dfork()) == 0) {
			/* lhs */
			close(pip[0]);
			dup2(pip[1], 1);
			eval(cmd->left);
			_exit(0);
		}
		close(pip[1]);
		dup2(pip[0], 0);
		status = eval(cmd->right);
		waitsh(pl);
		_exit(status);
	}
	return waitsh(pr);
}

static struct looploc {
	struct looploc *next;
	jmp_buf loc;
} *loops;

static int poploop(int lvl, int reason)
{
	struct looploc *jmppnt;

	if (!loops)
		return 0;

	while (--lvl && loops->next) {
		loops = loops->next;
	}
	jmppnt = loops;
	loops = loops->next;

	longjmp(jmppnt->loc, reason);
}

void unwindloops(void)
{
	while (loops)
		loops = loops->next;
}

static int evalloop(struct cmd *c)
{
	int mod, reason;
	struct looploc here;
	struct cloop *cmd;

	cmd = (struct cloop*)c;
	mod = cmd->type == CWHILE;

	if ((reason = setjmp(here.loc))) {
		exitstatus = 0;
		if (reason == SKIPBREAK)
			goto brk;
	}
	here.next = loops;
	loops = &here;


	while (eval(cmd->cond) ^ mod)
		exitstatus = eval(cmd->body);

brk:
	return exitstatus;
}

int break_builtin(struct cexec *cmd)
{
	int n = cmd->argc > 1 ? atoi(cmd->argv[1]) : 1;
	int type;

	if (n <= 0)
		raiseerr("break: bad number: %s", cmd->argv[1]);
	type = (cmd->argv[0][0] == 'c') ? SKIPCONT : SKIPBREAK;
	return poploop(n, type);
}

/*
 * fork and die on failure
 */
static pid_t dfork()
{
	pid_t pid = fork();
	if (pid < 0)
		die("fork:");
	if (pid == 0) {
		forked++;
		sigclearmask();
		closescript();
	}
	return pid;
}

/*
 * forks and execs the program
 *
 * The parent waits until the child program is done.
 * Gets the return value by waitpid.
 */
int runprog(struct cexec *cmd)
{
	pid_t pid = dfork();

	/* child */
	if (pid == 0) {
		execvp(cmd->argv[0], cmd->argv);
		/* if error */
		sdie(127, "%s: command not found", cmd->argv[0]);
	}

	/* parent */
	return waitsh(pid);
}

int waitsh(pid_t pid) {
	int status;

	INTOFF;
again:
	if (waitpid(pid, &status, WUNTRACED) < 0) {
		if (errno == EINTR)
			goto again;
		else
			die("%d:", pid);
	}
	INTON;

	if (WIFEXITED(status))
		return WEXITSTATUS(status);

	if (WIFSIGNALED(status)) {
		int sig = WTERMSIG(status);
		if (sig != SIGINT)
			perrorf("%d %s", pid, strsignal(sig));
		return 128 + sig;
	}

	if (WIFSTOPPED(status)) {
		perrorf("%d suspended", pid);
		return 128 + WSTOPSIG(status);
	}

	perrorf("Unexpected status (0x%x)\n", status);
	return status & 0xff;
}
