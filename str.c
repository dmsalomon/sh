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
