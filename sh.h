
#ifndef SH_H
#define SH_H

#include "cmd.h"

extern int rootpid;

int repl(void);
int source_builtin(struct cexec *);

#endif
