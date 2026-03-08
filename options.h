/** \file options.h
 */

#include "cmd.h"

struct shparam {
  int mallocd;
  int argc;
  char **argv;
};

extern struct shparam shparam;

void free_shparam(void);

int set_builtin(struct cexec *);

