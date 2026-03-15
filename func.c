/** \file var.c
 *
 */

#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "cmd.h"
#include "func.h"
#include "mem.h"

#define FUNCTABSIZE 11
static struct funcentry *functab[FUNCTABSIZE];

struct funcentry *lookupfunc(const char *name, int add) {
  unsigned int hashval;
  const char *p;
  struct funcentry *fp, **pp;

  p       = name;
  hashval = (unsigned char)*p << 4;
  while (*p)
    hashval += (unsigned char)*p++;
  hashval &= 0x7FFF;
  pp = &functab[hashval % FUNCTABSIZE];
  for (fp = *pp; fp; fp = fp->next) {
    if (strcmp(fp->func->name, name) == 0)
      break;
    pp = &fp->next;
  }
  if (add && !fp) {
    fp = *pp = xmalloc(sizeof(struct funcentry));
    fp->func = NULL;
    fp->next = NULL;
  }
  return fp;
}

void defunc(struct cfunc *cf) {
  struct funcentry *fp = lookupfunc(cf->name, 1);
#ifdef DEBUG
  assert(fp && "this function should be adding an entry");
#endif
  if (fp->func) {
    freecmd((struct cmd *)fp->func);
  }
  fp->func = (struct cfunc *)copycmd((struct cmd *)cf);
}
