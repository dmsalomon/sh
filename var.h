
#ifndef VAR_H
#define VAR_H

#include "cmd.h"

#define VEXPORT (1 << 1)
#define VRDONLY (1 << 2)
#define VSTSTAT (1 << 3) /* statically allocated struct */
#define VTXSTAT (1 << 4) /* statically allocated text */
#define VSTACK  (1 << 5) /* stack allocated text */
#define VUNSET  (1 << 6) /* variable is not set */
#define VNOFUNC (1 << 7)
#define VNOSET  (1 << 8)
#define VNOSAVE (1 << 9)

struct var {
  struct var *next;
  int flags;
  char *text;
  void (*func)(const char *); /* callback function */
};

extern struct var varinit[];

void initvar(void);
struct var *setvar(const char *, const char *, int);
struct var *setvareq(char *, int);
char *lookupvar(const char *);
int varcmp(const char *, const char *);
void unsetvar(const char *);
char **listvars(int on, int off, char ***end);

struct localframe;
struct localframe *pushlocalframe(int);
void unwindlocalvars(struct localframe *stop);
void setlocalvar(char *var, int flags);

int export_builtin(int argc, char **argv);
int read_builtin(int argc, char **argv);
int unset_builtin(int argc, char **argv);
int local_builtin(int argc, char **argv);

#define vps1    varinit[0]
#define vps2    (&vps1)[1]
#define vps4    (&vps2)[1]
#define vlineno (&vps4)[1]
#define vifs    (&vlineno)[1]
#define vppid   (&vifs)[1]

#define ps1val    (vps1.text + 4)
#define ps2val    (vps2.text + 4)
#define ps4val    (vps4.text + 4)
#define linenoval (vlineno.text + 7)
#define IFS       (vifs.text + 4)

static inline int varequal(const char *p, const char *q) {
  return !varcmp(p, q);
}

#define environment() listvars(VEXPORT, VUNSET, 0)

#endif
