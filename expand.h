
#ifndef EXPAND_H
#define EXPAND_H

#define EXP_FULL    (1 << 1)
#define EXP_NOSPLIT (1 << 2)

struct arg *expandargs(struct arg *, int flags);
struct arg *expandarg(struct arg *, struct arg **, int flags);
char *exparg(struct arg *arg);

#endif
