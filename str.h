/*
 * \file str.h
 */

#ifndef STR_H
#define STR_H

#include <ctype.h>
#include <stdint.h>

#define is_name(c)    ((c) == '_' || isalpha((unsigned char)(c)))
#define is_in_name(c) ((c) == '_' || isalnum((unsigned char)(c)))

int isassignment(const char *word);
char *endofname(const char *s);
static inline int goodname(const char *p) { return !*endofname(p); }

void badnum(const char *) __attribute__((noreturn));
int atomax10(const char *, intmax_t *);
int number(const char *);

#endif
