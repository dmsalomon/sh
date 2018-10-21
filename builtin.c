/** \file builtin.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtin.h"
#include "cmd.h"
#include "util.h"

#define LEN(a)		(sizeof(a) / sizeof(a[0]))
#define N_BUILTINS	LEN(builtins)

static int cd_builtin(struct cmd*);
static int exit_builtin(struct cmd*);
static int exec_builtin(struct cmd*);

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
	{"cd", 	 cd_builtin},
	{"exit", exit_builtin},
	{"exec", exec_builtin},
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
static int cd_builtin(struct cmd *cmd)
{
	char *home;

	if (cmd->argc > 2) {
		perrorf("cd: too many args");
		return 1;
	}

	if (cmd->argc == 2) {
		if (chdir(cmd->argv[1])) {
			perrorf("cd: %s:", cmd->argv[1]);
			return 1;
		}
	}
	else if ((home = getenv("HOME"))) {
		if (chdir(home)) {
			perrorf("cd: %s:", home);
			return 1;
		}
	}
	else {
		perrorf("cd: no directory");
		return 1;
	}

	return 0;
}

/* quits the shell
 * by default the status will be 0
 * the user can specify a status
 */
static int exit_builtin(struct cmd *cmd)
{
	int status = 0;

	if (cmd->argc > 1)
		status = atoi(cmd->argv[1]);

	exit(status);
}

/* exec a program to replace the shell */
static int exec_builtin(struct cmd *cmd)
{
	if (cmd->argc > 1) {
		scan_redir(cmd);
		execvp(cmd->argv[1], cmd->argv+1);
		/* if error */
		perrorf("exec: %s: command not found", cmd->argv[1]);
		return 127;
	}
	return 0;
}

