/** \file cmd.h
 */

#ifndef CMD_H
#define CMD_H

#include <stdio.h>

/* holds the argv and argc for a command */
struct cmd {
	int argc;
	char **argv;
};

int getcmd(struct cmd**);
struct cmd *parsecmd(char *line);
void freecmd(struct cmd *cmd);

int runcmd(struct cmd *);
void scan_redir(struct cmd*);
void redirect(char, char*, int);

#endif
