/** \file cmd.c
 *
 * evaluate commands.
 */

#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "builtin.h"
#include "cmd.h"
#include "error.h"
#include "eval.h"
#include "mem.h"
#include "output.h"
#include "redir.h"

int exitstatus;

static pid_t dfork(void);
static int evalpipe(struct cmd *);

int eval(struct cmd *c)
{
	struct cexec *ce;
	struct cif   *ci;
	struct cloop *clp;
	struct cbinary *cb;
	struct credir *cr;
	struct cunary *cu;

	struct jmploc *savehandler;
	struct jmploc jmploc;

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
		clp = (struct cloop*)c;

		while (eval(clp->cond) ^ (clp->type == CWHILE))
			exitstatus = eval(clp->body);

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

/*
 * fork and die on failure
 */
static pid_t dfork()
{
	pid_t pid = fork();
	if (pid < 0)
		die("fork:");
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

/*
 * opens the file for redirection to the specified
 * fd, or to the default fd for that op
 */
void redirect(char op, char *file, int fd)
{
	int mode, ofd;

	switch (op) {
	case '<':
		if (!fd) fd = 0;
		mode = O_RDONLY;
		break;
	case '>':
		if (!fd) fd = 1;
		mode = O_CREAT | O_TRUNC | O_WRONLY;
		break;
	case '+':
		if (!fd) fd = 1;
		mode = O_CREAT | O_APPEND | O_WRONLY;
		break;
	default:
		// should never happen
		die("expecting redirection");
		break;
	}

	/* open file for redirection
	 * use rw-r-r permissions */
	ofd = open(file, mode, 0644);

	if (ofd < 0)
		die("%s:", file);

	if (fd != ofd) {
		if (dup2(ofd, fd) < 0)
			die("%d:", fd);

		/* cleanup */
		if (close(ofd) < 0)
			die("%d:", ofd);
	}
}

int waitsh(int pid) {
	int status;

	if (waitpid(pid, &status, WUNTRACED) < 0)
		die("%d:", pid);

	if (WIFEXITED(status))
		return WEXITSTATUS(status);
	if (WIFSIGNALED(status))
		return 128 + WTERMSIG(status);

	if (WIFSTOPPED(status)) {
		reportf("%d suspended\n", pid);
		return 128 + WTERMSIG(status);
	}

	perrorf("Unexpected status (0x%x)\n", status);
	return status & 0xff;
}
