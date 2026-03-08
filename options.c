/** \file options.c
 */

#include "options.h"
#include "mem.h"

struct shparam shparam = {
    .argc    = 0,
    .argv    = 0,
    .mallocd = 0,
};

static void copy_to_shparam(int argc, char **argv);

int set_builtin(struct cexec *c) {
  free_shparam();
  copy_to_shparam(c->argc, c->argv);
  return 0;
}

static void copy_to_shparam(int argc, char **argv) {
  shparam.argc    = argc - 1;
  shparam.argv    = xmalloc(shparam.argc * sizeof(*shparam.argv));
  shparam.mallocd = 1;

  for (int i = 0; i < shparam.argc; i++) {
    shparam.argv[i] = xstrdup(argv[i + 1]);
  }
}

void free_shparam() {
  if (!shparam.mallocd)
    return;

  for (int i = 0; i < shparam.argc; i++)
    if (shparam.argv[i])
      free(shparam.argv[i]);

  free(shparam.argv);
}
