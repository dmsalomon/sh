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
static int echo_builtin(struct cexec *);
static int exit_builtin(struct cexec *);
static int exec_builtin(struct cexec *);
static int fg_builtin(struct cexec *);
static int true_builtin(struct cexec *);
static int args_builtin(struct cexec *);
static int builtin_builtin(struct cexec *);
static int command_builtin(struct cexec *);

/*
 * A struct to hold the builtin
 * names with their corresponding
 * functions
 */
static struct builtin {
	char *name;
	builtin_func f;
}
/* must be listed in alphabetical order for bsearch */
builtins[] = {
	{".",        source_builtin},
	{"args",     args_builtin},
	{"break",    break_builtin},
	{"builtin",  builtin_builtin},
	{"cd", 	     cd_builtin},
	{"command",  command_builtin},
	{"continue", break_builtin},
	{"echo",     echo_builtin},
	{"eval",     eval_builtin},
	{"exec",     exec_builtin},
	{"exit",     exit_builtin},
	{"export",   export_builtin},
	{"false",    true_builtin},
	{"fg",       fg_builtin},
	{"read",     read_builtin},
	{"readonly", export_builtin},
	{"source",   source_builtin},
	{"true",     true_builtin},
};

static int builtincmp(const void *v1, const void *v2)
{
	struct builtin *b1 = (struct builtin*)v1;
	struct builtin *b2 = (struct builtin*)v2;
	return strcmp(b1->name, b2->name);
}

/*
 * return the builtin function associated
 * with a name, or NULL if not found
 */
builtin_func get_builtin(char *name)
{
	struct builtin *b;
	b = bsearch(&name, builtins, N_BUILTINS, sizeof(struct builtin), builtincmp);
	return b ? b->f : NULL;
}

static int echo_builtin(struct cexec *cmd)
{
	int i = 1;

	while (i < cmd->argc) {
		printf("%s", cmd->argv[i]);
		if (++i < cmd->argc)
			printf(" ");
	}
	printf("\n");
	fflush(stdout);

	return 0;
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

/* print the argument,
 * useful for debugging */
static int args_builtin(struct cexec *cmd)
{
	char **app;

	printf("argc: %d\n", cmd->argc-1);
	for (app=cmd->argv+1; *app; app++) {
		printf("`%s`: %ld\n", *app, strlen(*app));
	}

	return 0;
}

static int builtin_builtin(struct cexec *cmd)
{
	cmd->argv++;
	cmd->argc--;

	if (cmd->argc == 0)
		return 0;

	builtin_func bt = get_builtin(cmd->argv[0]);

	if (!bt) {
		perrorf("builtin: %s: not a shell builtin", cmd->argv[0]);
		return 1;
	}

	return (*bt)(cmd);
}

static int command_builtin(struct cexec *cmd)
{
	cmd->argc--;
	cmd->argv++;

	if (cmd->argc == 0)
		return 0;

	return runprog(cmd);
}
