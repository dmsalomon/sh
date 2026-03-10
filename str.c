/*
 * \file str.c
 */

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>

#include "error.h"
#include "str.h"

void badnum(const char *s) { raiseerr("illegal number: %s", s); }

char *endofname(const char *name) {
  char *p;

  p = (char *)name;
  if (!is_name(*p))
    return p;
  while (*++p)
    if (!is_in_name(*p))
      break;
  return p;
}

int isassignment(const char *word) {
  char *p;

  p = endofname(word);
  return p != word && *p == '=';
}

/*
 * Convert a string into an integer of type intmax_t.  Alow trailing spaces.
 */
int atomax(const char *s, int base, intmax_t *r) {
  char *p;

  errno = 0;

  *r = strtoimax(s, &p, base);

  if (errno == ERANGE)
    return -1;

  // don't allow blank strings
  if (p == s && base)
    return -1;

  while (isspace((unsigned char)*p))
    p++;

  if (*p)
    return -1;

  return 0;
}

int atomax10(const char *s, intmax_t *r) { return atomax(s, 10, r); }

int number(const char *s) {
  intmax_t n;

  if (atomax10(s, &n) || n < 0 || n > INT_MAX)
    badnum(s);

  return n;
}

int glob_match(const char *str, const char *pat) {
  const char *star  = 0;
  const char *match = 0;

  while (*str) {
    if (*pat == '?' || *pat == *str) {
      str++;
      pat++;
    } else if (*pat == '*') {
      star = pat;

      /* collapse consecutive '*' */
      while (*pat == '*')
        pat++;

      match = str;
    } else if (star) {
      pat = star + 1;
      str = ++match;
    } else {
      return 0;
    }
  }

  /* allow trailing '*' to match empty */
  while (*pat == '*')
    pat++;

  return *pat == '\0';
}
