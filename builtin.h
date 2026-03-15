/** \file builtin.h
 */

#ifndef BUILTIN_H
#define BUILTIN_H

#include "cmd.h"

#define BUILTIN_SPECIAL (1 << 1)
#define BUILTIN_ASSIGN  (1 << 2)

typedef int (*builtin_func)(int argc, char** argv);

int args_builtin(int argc, char **argv);
int builtin_builtin(int argc, char **argv);
int cd_builtin(int argc, char **argv);
int command_builtin(int argc, char **argv);
int echo_builtin(int argc, char **argv);
int exec_builtin(int argc, char **argv);
int exit_builtin(int argc, char **argv);
int fg_builtin(int argc, char **argv);
int tokens_builtin(int argc, char **argv);
int true_builtin(int argc, char **argv);

struct builtin {
  char *name;
  builtin_func func;
  int flags;
};

struct builtin *get_builtin(const char *);

#endif
