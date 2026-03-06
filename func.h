
#ifndef FUNC_H
#define FUNC_H

#include "cmd.h"

struct funcentry {
  struct cfunc *func;
	struct funcentry *next;
};

struct funcentry *lookupfunc(const char *, int);
void defunc(struct cfunc *);

#endif // FUNC_H
