/*
 * \file str.h
 */

#include <ctype.h>
#include <stdint.h>

#define is_name(c)    ((c) == '_' || isalpha((unsigned char)(c)))
#define is_in_name(c) ((c) == '_' || isalnum((unsigned char)(c)))

void badnum(const char *) __attribute__((noreturn));
int atomax10(const char *, intmax_t *);
int number(const char *);
