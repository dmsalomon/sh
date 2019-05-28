/** \file eval.h
 */

#ifndef EVAL_H
#define EVAL_H

extern int forked;
extern int exitstatus;

#include "cmd.h"

int eval(struct cmd *);
int evalcmd(struct cexec *);
int evalstring(char *s);
int runprog(struct cexec *);
pid_t dfork(void);
int waitsh(int);

void unwindloops(void);
int break_builtin(struct cexec *);
int eval_builtin(struct cexec *);

/* must be <0 */
#define SKIPBREAK 1
#define SKIPCONT  2

#endif
