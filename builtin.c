/** \file builtin.c
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtin.h"
#include "cmd.h"
#include "eval.h"
#include "output.h"
#include "sh.h"
#include "var.h"

#define LEN(a)		(sizeof(a) / sizeof(a[0]))
#define N_BUILTINS	LEN(builtins)

static int cd_builtin(struct cexec *);
static int exit_builtin(struct cexec *);
static int exec_builtin(struct cexec *);
static int fg_builtin(struct cexec *);
static int true_builtin(struct cexec *);

/*
 * A struct to hold the builtin
 * names with their corresponding
 * functions
 */
static struct {
	char *name;
	builtin_func f;
}
builtins[] = {
	{".",        source_builtin},
	{"break",    break_builtin},
	{"cd", 	     cd_builtin},
	{"continue", break_builtin},
	{"eval",     eval_builtin},
	{"exec",     exec_builtin},
	{"export",   export_builtin},
	{"exit",     exit_builtin},
	{"false",    true_builtin},
	{"fg",       fg_builtin},
	{"readonly", export_builtin},
	{"source",   source_builtin},
	{"true",     true_builtin},
};

/*
 * return the builtin function associated
 * with a name, or NULL if not found
 */
builtin_func get_builtin(char *name)
{
	for (int i = 0; i < N_BUILTINS; i++)
		if (strcmp(name, builtins[i].name) == 0)
			return builtins[i].f;
	return NULL;
}

/* change directory
 *
 * if no args are specified the shell
 * will try to cd to $HOME
 */

static int cd_builtin(struct cexec *cmd)
{
	int print = 0;
	char *pwd, *dest;

	if (cmd->argc > 2) {
		perrorf("cd: too many args");
		return 1;
	}

	dest = cmd->argv[1];
	if (!dest) {
		dest = lookupvar("HOME");
	} else if (dest[0] == '-' && dest[1] == '\0') {
		dest = lookupvar("OLDPWD");
		print = 1;
	}
	if (!dest) {
		perrorf("cd: no directory");
		return 1;
	}
	pwd = lookupvar("PWD");

	if (chdir(dest)) {
		perrorf("cd: %s:", dest);
		return 1;
	}
	if (print)
		printf("%s\n", dest);

	setvar("OLDPWD", pwd, VEXPORT);
	setvar("PWD", getcwd(0, 0), VNOSAVE | VEXPORT);

	return 0;
}

/* quits the shell
 * by default the status will be 0
 * the user can specify a status
 */
static int exit_builtin(struct cexec *cmd)
{
	int status = 0;

	if (cmd->argc > 1)
		status = atoi(cmd->argv[1]);

	_exit(status);
}

/* exec a program to replace the shell */
static int exec_builtin(struct cexec *cmd)
{
	if (cmd->argc > 1) {
		execvp(cmd->argv[1], cmd->argv+1);
		/* if error */
		perrorf("exec: %s: command not found", cmd->argv[1]);
		return 127;
	}
	return 0;
}

static int fg_builtin(struct cexec *cmd)
{
	if (cmd->argc < 2) {
		perrorf("fg: too few args");
		return 1;
	}

	int pid = atoi(cmd->argv[1]);

	if(kill(pid, SIGCONT)) {
		perrorf("fg: cannot resume %d", pid);
		return 1;
	}

	return waitsh(pid);
}

static int true_builtin(struct cexec *cmd)
{
	return cmd->argv[0][0] == 'f';
}

