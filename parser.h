
#ifndef PARSER_H
#define PARSER_H

#include <ctype.h>

struct cmd *parseline(void);

int isassignment(const char *word);
char *endofname(const char *);

#define is_name(c)    ((c) == '_' || isalpha((unsigned char)(c)))
#define is_in_name(c) ((c) == '_' || isalnum((unsigned char)(c)))

static inline int goodname(const char *p)
{
	return !*endofname(p);
}

#endif
