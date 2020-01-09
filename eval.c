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
#include "expand.h"
#include "input.h"
#include "mem.h"
#include "output.h"
#include "redir.h"
#include "parser.h"
#include "sh.h"
#include "trap.h"
#include "var.h"

int exitstatus;
int forked;

static int evalpipe(struct cmd *);
static int evalloop(struct cmd *);
static int evalfor(struct cmd *);

int eval(struct cmd *c)
{
	pid_t pid;

	struct cexec   *ce;
	struct cif     *ci;
	struct cbinary *cb;
	struct credir  *cr;
	struct cunary  *cu;

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
		} else {
			exitstatus = 2;
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

	case CBGND:
		cb = (struct cbinary*)c;

		if (dfork() == 0)
			_exit(eval(cb->left));
		if (cb->right)
			exitstatus = eval(cb->right);
		else
			exitstatus = 0;
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

	case CFOR:
		exitstatus = evalfor(c);
		break;

	case CBRC:
		cu = (struct cunary*)c;
		exitstatus = eval(cu->cmd);
		break;

	case CSUB:
		cu = (struct cunary*)c;
		if ((pid = dfork()) == 0)
			_exit(eval(cu->cmd));
		exitstatus = waitsh(pid);
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
	char argc, **argv;
	struct arg *expargs, *ap;

	argc = 0;
	expargs = expandargs(cmd->args);

	for (ap = expargs; ap && isassignment(ap->text); ap = ap->next)
		setvareq(ap->text, 0);

	expargs = ap;
	for (; ap; ap = ap->next) {
		argc++;
	}

	if (argc == 0)
		return 0;

	argv = stalloc(sizeof(*argv)*(argc+1));
	for (i = 0, ap = expargs; i < argc; i++, ap = ap->next)
		argv[i] = ap->text;
	argv[i] = NULL;
	cmd->argv = argv;
	cmd->argc = argc;

	builtin_func bt = get_builtin(cmd->argv[0]);

	if (bt)
		return (*bt)(cmd);
	else
		return runprog(cmd);
}

int evalstring(char *s)
{
	int status;

	s = sstrdup(s);
	setinputstring(s, INPUT_PUSH_FILE);
	status = repl();
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
			concat = stputs(*ap, concat);
			STPUTC(' ', concat);
		}
		if (concat > stacknext)
			concat--;
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
			close(pip[1]);
			close(1);
			_exit(0);
		}
		close(pip[1]);
		dup2(pip[0], 0);
		status = eval(cmd->right);
		/* close to cause SIGPIPE */
		close(pip[0]);
		close(0);
		waitsh(pl);
		_exit(status);
	}
	return waitsh(pr);
}

static struct looploc {
	struct looploc *next;
	jmp_buf loc;
} *loops;

static int poploop(int lvl, int type)
{
	struct looploc *jmppnt;

	if (!loops)
		return 0;

	while (--lvl && loops->next) {
		loops = loops->next;
	}
	jmppnt = loops;
	loops = loops->next;

	exitstatus = 0;
	longjmp(jmppnt->loc, type);
}

void unwindloops(void)
{
	loops = NULL;
}

static int evalloop(struct cmd *c)
{
	int mod, type;
	struct looploc here;
	struct cloop *cmd;
	struct stackmark mark;

	cmd = (struct cloop*)c;
	mod = cmd->type == CWHILE;

	if ((type = setjmp(here.loc))) {
		popstackmark(&mark);
		if (type == SKIPBREAK)
			goto brk;
	}
	here.next = loops;
	loops = &here;

	pushstackmark(&mark);

	while (eval(cmd->cond) ^ mod) {
		exitstatus = eval(cmd->body);
		if (exitstatus == 128 + SIGPIPE)
			break;
		popstackmark(&mark);
	}

brk:
	loops = here.next;
	popstackmark(&mark);
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

static int evalfor(struct cmd *c)
{
	int type;
	struct cfor *cmd;
	struct arg *lp, *explist;
	struct stackmark fmark, mark;
	struct looploc here;

	cmd = (struct cfor*)c;
	pushstackmark(&fmark);

	if (!(explist = expandargs(cmd->list))) {
		popstackmark(&fmark);
		return exitstatus = 0;
	}

	here.next = loops;
	loops = &here;
	pushstackmark(&mark);

	for (lp = explist; lp; lp = lp->next) {
		if ((type = setjmp(here.loc))) {
			popstackmark(&mark);
			if (type == SKIPBREAK)
				break;
			else
				continue;
		}
		setvar(cmd->var, lp->text, 0);
		exitstatus = eval(cmd->body);
		popstackmark(&mark);
	}

	loops = here.next;
	popstackmark(&fmark);
	return exitstatus;
}

/*
 * fork and die on failure
 */
pid_t dfork()
{
	pid_t pid = fork();
	if (pid < 0)
		die("fork:");
	if (pid == 0) {
		forked++;
		sigclearmask();
		closescript();
		FORCEINTON;
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
	int status;
	pid_t pid;

	INTOFF;
	if ((pid = dfork()) == 0) {
		/* child */
		execvp(cmd->argv[0], cmd->argv);
		/* if error */
		sdie(127, "%s: command not found", cmd->argv[0]);
	}

	/* parent */
	status = waitsh(pid);
	INTON;
	return status;
}

int waitsh(pid_t pid)
{
	int status;

	INTOFF;
again:
	if (waitpid(pid, &status, WUNTRACED) < 0) {
		if (errno == EINTR)
			goto again;
		else if (!forked)
			die("%d:", pid);
		else
			_exit(1);
	}
	INTON;

	if (WIFEXITED(status))
		return WEXITSTATUS(status);

	if (WIFSIGNALED(status)) {
		int sig = WTERMSIG(status);
		if (sig != SIGINT && !forked)
			perrorf("%d %s", pid, strsignal(sig));
		return 128 + sig;
	}

	if (WIFSTOPPED(status)) {
		if (!forked)
			perrorf("%d suspended", pid);
		return 128 + WSTOPSIG(status);
	}

	if (!forked)
		perrorf("Unexpected status (0x%x)\n", status);
	return status & 0xff;
}

