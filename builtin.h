/** \file builtin.h
 */

#ifndef BUILTIN_H
#define BUILTIN_H

#include "cmd.h"

#define BUILTIN_SPECIAL (1 << 1)
#define BUILTIN_ASSIGN  (1 << 2)

typedef int (*builtin_func)(struct cexec *);

int args_builtin(struct cexec *);
int builtin_builtin(struct cexec *);
int cd_builtin(struct cexec *);
int command_builtin(struct cexec *);
int echo_builtin(struct cexec *);
int exec_builtin(struct cexec *);
int exit_builtin(struct cexec *);
int fg_builtin(struct cexec *);
int tokens_builtin(struct cexec *);
int true_builtin(struct cexec *);

struct builtin {
  char *name;
  builtin_func func;
  int flags;
};

struct builtin *get_builtin(const char *);

#endif
