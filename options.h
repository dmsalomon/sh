/** \file options.h
 */

#include "cmd.h"

struct shparam {
  int mallocd;
  int np;
  char **p;
};

extern char *arg0;
extern struct shparam shparam;
extern char *minusc;

#define sflag optlist[0]
#define xflag optlist[1]

#define NOPTS 2

extern const char optletters[NOPTS];
extern char optlist[NOPTS];


int procargs(int, char **);
void freeparam(struct shparam *);

int set_builtin(struct cexec *);
int shift_builtin(struct cexec *);
