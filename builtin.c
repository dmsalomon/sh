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
#include "lexer.h"
#include "options.h"
#include "output.h"
#include "str.h"
#include "var.h"

#define LEN(a)     (sizeof(a) / sizeof(a[0]))
#define N_BUILTINS LEN(builtins)

/* must be listed in alphabetical order for bsearch */
static struct builtin builtins[] = {
    {".",        source_builtin,  BUILTIN_SPECIAL                 },
    {":",        true_builtin,    BUILTIN_SPECIAL                 },
    {"args",     args_builtin,    0                               },
    {"break",    break_builtin,   BUILTIN_SPECIAL                 },
    {"builtin",  builtin_builtin, 0                               },
    {"cd",       cd_builtin,      0                               },
    {"command",  command_builtin, 0                               },
    {"continue", break_builtin,   BUILTIN_SPECIAL                 },
    {"echo",     echo_builtin,    0                               },
    {"eval",     eval_builtin,    BUILTIN_SPECIAL                 },
    {"exec",     exec_builtin,    BUILTIN_SPECIAL                 },
    {"exit",     exit_builtin,    BUILTIN_SPECIAL                 },
    {"export",   export_builtin,  BUILTIN_SPECIAL | BUILTIN_ASSIGN},
    {"false",    true_builtin,    0                               },
    {"fg",       fg_builtin,      0                               },
    {"local",    local_builtin,   BUILTIN_SPECIAL | BUILTIN_ASSIGN},
    {"read",     read_builtin,    0                               },
    {"readonly", export_builtin,  BUILTIN_SPECIAL | BUILTIN_ASSIGN},
    {"return",   return_builtin,  BUILTIN_SPECIAL                 },
    {"set",      set_builtin,     BUILTIN_SPECIAL                 },
    {"shift",    shift_builtin,   BUILTIN_SPECIAL                 },
    {"source",   source_builtin,  0                               },
    {"tokens",   tokens_builtin,  0                               },
    {"true",     true_builtin,    0                               },
    {"unset",    unset_builtin,   BUILTIN_SPECIAL                 },
};

static int builtincmp(const void *v1, const void *v2) {
  struct builtin *b1 = (struct builtin *)v1;
  struct builtin *b2 = (struct builtin *)v2;
  return strcmp(b1->name, b2->name);
}

/*
 * return the builtin function associated
 * with a name, or NULL if not found
 */
struct builtin *get_builtin(const char *name) {
  return bsearch(&name, builtins, N_BUILTINS, sizeof(struct builtin),
                 builtincmp);
}

int echo_builtin(struct cexec *cmd) {
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

int cd_builtin(struct cexec *cmd) {
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
int exit_builtin(struct cexec *cmd) {
  _exit(cmd->argc > 1 ? number(cmd->argv[1]) : exitstatus);
}

/* exec a program to replace the shell */
int exec_builtin(struct cexec *cmd) {
  if (cmd->argc > 1) {
    execvp(cmd->argv[1], cmd->argv + 1);
    /* if error */
    perrorf("exec: %s: command not found", cmd->argv[1]);
    return 127;
  }
  return 0;
}

int fg_builtin(struct cexec *cmd) {
  if (cmd->argc < 2) {
    perrorf("fg: too few args");
    return 1;
  }

  int pid = number(cmd->argv[1]);

  if (kill(pid, SIGCONT)) {
    perrorf("fg: cannot resume %d", pid);
    return 1;
  }

  return waitsh(pid);
}

int true_builtin(struct cexec *cmd) { return cmd->argv[0][0] == 'f'; }

/* print the argument,
 * useful for debugging */
int args_builtin(struct cexec *cmd) {
  char **app;

  printf("argc: %d\n", cmd->argc - 1);
  for (app = cmd->argv + 1; *app; app++) {
    printf("`%s`: %ld\n", *app, strlen(*app));
  }

  return 0;
}

int builtin_builtin(struct cexec *cmd) {
  cmd->argv++;
  cmd->argc--;

  if (cmd->argc == 0)
    return 0;

  struct builtin *b = get_builtin(cmd->argv[0]);

  if (!b) {
    perrorf("builtin: %s: not a shell builtin", cmd->argv[0]);
    return 1;
  }
  builtin_func bt = b->func;

  return (*bt)(cmd);
}

int command_builtin(struct cexec *cmd) {
  cmd->argc--;
  cmd->argv++;

  if (cmd->argc == 0)
    return 0;

  return runprog(cmd);
}

int tokens_builtin(struct cexec *cmd) {
  (void)cmd;
  show_tokens = !show_tokens;
  return 0;
}
