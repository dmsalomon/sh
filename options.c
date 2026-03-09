/** \file options.c
 */

#include <assert.h>
#include <stdlib.h>

#include "error.h"
#include "input.h"
#include "mem.h"
#include "options.h"
#include "str.h"

char *arg0;
char *minusc;
struct shparam shparam = {0};

// static const char *const optnames[NOPTS] = {
//     "stdin",
//     "xtrace",
//     "verbose",
// };

const char optletters[NOPTS] = {
    's',
    'x',
    'v',
};

char optlist[NOPTS];

static void setoption(char, int);
static void setparam(char **argv);

int options(char ***argv, int cmdline) {
  int val;
  int login = 0;

  char c;
  char **xargv = *argv;
  char *p;

  if (cmdline)
    minusc = NULL;

  while ((p = *xargv)) {
    xargv++;
    if ((c = *p++) == '-') {
      val = 1;
      // either - or --
      if (p[0] == '\0' || (p[0] == '-' && p[1] == '\0')) {
        if (!cmdline) {
          // single dash means turn off -x and -v
          if (p[0] == '\0')
            xflag = vflag = 0;
          else if (!*xargv)
            // reset params
            setparam(xargv);
        }
        break;
      }
    } else if (c == '+') {
      val = 0;
    } else {
      xargv--;
      break;
    }
    while ((c = *p++) != '\0') {
      if (c == 'c' && cmdline) {
        minusc = p; /* command is after shell args*/
      } else if (c == 'l' && cmdline) {
        login = 1;
        // } else if (c == 'o') {
        //   minus_o(*argptr, val);
        //   if (*argptr)
        //     argptr++;
      } else {
        setoption(c, val);
      }
    }
  }
  *argv = xargv;
  return login;
}

static void setoption(char flag, int val) {
  int i;

  for (i = 0; i < NOPTS; i++)
    if (optletters[i] == flag) {
      optlist[i] = val;
      // if (val) {
      //   /* #%$ hack for ksh semantics */
      //   if (flag == 'V')
      //     Eflag = 0;
      //   else if (flag == 'E')
      //     Vflag = 0;
      // }
      return;
    }
  raiseerr("illegal option -%c", flag);
  /* unreachable */
}

int procargs(int argc, char **argv) {
  int login;

  login = argv[0] && argv[0][0] == '-';
  arg0  = argv[0];

  if (argc > 0)
    argv++;
  for (int i = 0; i < NOPTS; i++)
    optlist[i] = 2;

  login |= options(&argv, 1);

  for (int i = 0; i < NOPTS; i++)
    if (optlist[i] == 2)
      optlist[i] = 0;

  if (!*argv) {
    if (minusc)
      raiseerr("-c requires an argument");
    sflag = 1;
  }

  if (minusc) {
    minusc = *argv++;
    if (*argv)
      goto setarg0;
  } else if (!sflag) {
    setinputfile(*argv, 0);
  setarg0:
    arg0 = *argv++;
    // commandname = arg0;
  }

  shparam.p = argv;
  assert(shparam.mallocd == 0 && shparam.np == 0);
  while (*argv) {
    shparam.np++;
    argv++;
  }

  return login;
}

int set_builtin(struct cexec *c) {
  char **argv = c->argv + 1;
  options(&argv, 0);
  if (*argv)
    setparam(argv);
  return 0;
}

int shift_builtin(struct cexec *c) {

  int n;
  char **ap1, **ap2;

  n = 1;
  if (c->argc > 1)
    n = number(c->argv[1]);
  if (n > shparam.np)
    raiseerr("can't shift that many");
  INTOFF;
  shparam.np -= n;
  for (ap1 = shparam.p; --n >= 0; ap1++) {
    if (shparam.mallocd)
      free(*ap1);
  }
  ap2 = shparam.p;
  while ((*ap2++ = *ap1++) != NULL)
    ;
  INTON;
  return 0;
}

static void setparam(char **argv) {
  int np;
  char **newparam;
  char **ap;

  for (np = 0; argv[np]; np++)
    ;

  ap = newparam = xmalloc((np + 1) * sizeof(*ap));
  while (*argv) {
    *ap++ = xstrdup(*argv++);
  }
  *ap = NULL;
  freeparam(&shparam);
  shparam.mallocd = 1;
  shparam.np      = np;
  shparam.p       = newparam;
}

void freeparam(struct shparam *param) {
  char **ap;

  if (param->mallocd) {
    if (param && param->mallocd) {
      for (ap = param->p; *ap; ap++)
        free(*ap);
      free(param->p);
    }
  }
}
