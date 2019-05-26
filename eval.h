/** \file eval.h
 */

#ifndef EVAL_H
#define EVAL_H

int eval(struct cmd *);
int evalcmd(struct cexec *);
int runprog(struct cexec *);
int waitsh(int);

#endif
