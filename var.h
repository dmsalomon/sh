
#ifndef VAR_H
#define VAR_H

#include "cmd.h"

#define VEXPORT (1 << 1)
#define VRDONLY (1 << 2)
#define VSTSTAT (1 << 3)
#define VTXSTAT (1 << 4)
#define VSTACK  (1 << 5)
#define VUNSET  (1 << 6)
#define VNOFUNC (1 << 7)
#define VNOSET  (1 << 8)
#define VNOSAVE (1 << 9)

struct var {
	struct var *next;
	int flags;
	char *text;
	void (*func)(const char *);
};

extern struct var varinit[];

void initvar(void);
struct var *setvar(const char *, const char *, int);
struct var *setvareq(char *, int);
char *lookupvar(const char *);
int varcmp(const char *, const char *);

int export_builtin(struct cexec *);
int read_builtin(struct cexec *);

#define vps1 varinit[0]
#define vps2 (&vps1)[1]
#define vps4 (&vps2)[1]
#define vlineno (&vps4)[1]
#define vifs (&vlineno)[1]

#define ps1val (vps1.text + 4)
#define ps2val (vps2.text + 4)
#define ps4val (vps4.text + 4)
#define linenoval (vlineno.text + 7)
#define IFS (vifs.text + 4)

static inline int varequal(const char *p, const char *q)
{
	return !varcmp(p, q);
}

#endif
