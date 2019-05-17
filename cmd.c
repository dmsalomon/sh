/** \file cmd.c
 *
 * Read, parse and run commands.
 */

#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "cmd.h"
#include "builtin.h"
#include "util.h"

#ifndef LINE_MAX
#ifdef _POSIX_LINE_MAX
#define LINE_MAX	_POSIX2_LINE_MAX
#else
#define LINE_MAX	4096
#endif
#endif

#define PROMPT		"$ "
#define WHITESPACE	" \t\r\n\v"

extern int lstatus;

int runprog(struct cmd*);
static char *expand_arg(char *);

/*
 * print the prompt to stderr
 * (this way stdout can be redirected
 *  without the prompt)
 *
 * Uses PS1 if set in the environment.
 */
static void print_prompt(void)
{
	if (!isatty(STDIN_FILENO)) return;

	char *p = getenv("PS1");
	fprintf(stderr, "%s", p ? p : PROMPT);
}

/*
 * get the command from the user
 *
 * the buffer to store the command line, and
 * the arguments after strtok runs, are kept
 * in a static buffer.
 *
 * return 1 if there was input, else 0
 */
int getcmd(struct cmd **pcmd)
{
	static char line[LINE_MAX];

	print_prompt();

	if (fgets(line, LINE_MAX, stdin) == NULL)
		return 0;

	*pcmd = parsecmd(line);
	return 1;
}

/*
 * parse the command line
 *
 * uses to strtok to build an argv
 */
struct cmd *parsecmd(char *line)
{
	struct cmd *cmd = malloc(sizeof(struct cmd));
	alloc_die(cmd);

	*cmd = (struct cmd) {.argc = 0, .argv = NULL};

	char *scratch;
	char *arg = strtok_r(line, WHITESPACE, &scratch);

	if (!arg) {
		free(cmd);
		return NULL;
	}

	while (arg) {
		cmd->argv = realloc(cmd->argv, sizeof(char *) * ++(cmd->argc));
		alloc_die(cmd->argv);
		cmd->argv[cmd->argc - 1] = expand_arg(arg);
		arg = strtok_r(NULL, WHITESPACE, &scratch);
	}

	/* null terminate argv for execvp */
	cmd->argv = realloc(cmd->argv, sizeof(char *) * (cmd->argc + 1));
	alloc_die(cmd->argv);
	cmd->argv[cmd->argc] = NULL;

	return cmd;
}

/*
 * runs the command
 *
 * if its a builtin it runs the corresponding funcion
 * otherwise it runs as a program
 *
 * return the program or builtin status
 */
int runcmd(struct cmd *cmd)
{
	int ret;

	builtin_func bt = get_builtin(cmd->argv[0]);

	if (bt)
		ret = (*bt)(cmd);
	else
		ret = runprog(cmd);

	freecmd(cmd);
	return ret;
}

/*
 * forks and execs the program
 *
 * The parent waits until the child program is done.
 * Gets the return value by waitpid.
 */
int runprog(struct cmd *cmd)
{
	pid_t pid = dfork();

	/* child */
	if (pid == 0) {
		scan_redir(cmd);
		execvp(cmd->argv[0], cmd->argv);
		/* if error */
		sdie(127, "%s: command not found", cmd->argv[0]);
	}

	/* parent */
	return waitsh(pid);
}

void freecmd(struct cmd *cmd)
{
	if (!cmd)
		return;

	if (cmd->argv)
		free(cmd->argv);

	free(cmd);
}

/*
 * expand each argument.
 *
 * for now it just checks for $?
 * and so a static buffer is enough
 * otherwise it would need to allocate
 * memory which would complicate things.
 */
static char *expand_arg(char *arg)
{
	static char status[32];
	static char pid[32];

	if (!arg || arg[0] != '$' || !arg[1] || arg[2])
		return arg;

	switch (arg[1]) {
	case '?':
		snprintf(status, sizeof(status), "%d", lstatus);
		return status;
	case '$':
		snprintf(pid, sizeof(pid), "%d", getpid());
		return pid;
	default:
		return arg;
	}
	return arg;
}

/*
 * setup the file redirection
 *
 * Will modify the argv to remove the
 * redirection operators and their associated
 * files.
 */
void scan_redir(struct cmd *cmd)
{
	int iarg = 0;
	char *arg;
	char *file;
	int fd;

	for (int i = 0; i < cmd->argc; i++) {
		arg = cmd->argv[i];
		fd = 0;

		/* get the fd for redirection if specified */
		while (isdigit(*arg)) {
			fd = 10*fd + (*arg - '0');
			arg++;
		}

		/* skip this argument if no arrow is found */
		if (!arg[0] || !strchr("<>", arg[0])) {
			if (iarg < i)
				cmd->argv[iarg] = cmd->argv[i];
			iarg++;
			continue;
		}

		/* if append */
		if (arg[0] == '>' && arg[1] == '>') {
			arg++;
			arg[0] = '+';
		}

		/* find the file: could be after op or next arg */
		if (arg[1])
			file = arg+1;
		else if (++i < cmd->argc)
			file = cmd->argv[i];
		else
			die("no redirection file specified");

		redirect(arg[0], file, fd);
	}

	cmd->argv[iarg] = NULL;
	cmd->argc = iarg;
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
