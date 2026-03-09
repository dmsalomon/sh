
#ifndef PARSER_H
#define PARSER_H

struct cmd *parseline(void);
struct cmd *parsesub(void);

int isassignment(const char *word);
char *endofname(const char *s);

static inline int goodname(const char *p) { return !*endofname(p); }

#endif
