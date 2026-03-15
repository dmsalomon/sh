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
#define vflag optlist[2]

#define NOPTS 3

extern const char optletters[NOPTS];
extern char optlist[NOPTS];

int procargs(char **);
void freeparam(struct shparam *);

int set_builtin(struct cexec *);
int shift_builtin(struct cexec *);
