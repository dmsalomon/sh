/** \file eval.h
 */

#ifndef EVAL_H
#define EVAL_H

#include "cmd.h"
#include <sys/types.h>

extern int exitstatus;
extern int forked;
extern char *commandname;

int eval(struct cmd *);
int evalcmd(struct cexec *);
int evalstring(char *s);
int runprog(char **argv);
pid_t dfork(void);
int waitsh(int);

void unwindloops(void);
void unwindrets(void);

int break_builtin(int argc, char **argv);
int eval_builtin(int argc, char **argv);
int return_builtin(int argc, char **argv);
int source_builtin(int argc, char **argv);

/* must be <0 */
#define SKIPBREAK 1
#define SKIPCONT  2

#endif
