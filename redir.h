
#ifndef REDIR_H
#define REDIR_H

#include "cmd.h"

extern int preverrfd;

int pushredirect(struct credir *);
void popredirect(void);
void unwindredir(void);
int savefd(int);

#endif
