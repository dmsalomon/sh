
#ifndef EXPAND_H
#define EXPAND_H

#define IFS " \n\t"

struct arg *expandargs(struct arg *);
char *expfilename(struct arg *);

#endif
