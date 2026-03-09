/* \file sh.c
 *
 * A simple shell
 */

#include <stdio.h>
#include <unistd.h>

#include "cmd.h"
#include "error.h"
#include "eval.h"
#include "input.h"
#include "lexer.h"
#include "mem.h"
#include "options.h"
#include "parser.h"
#include "redir.h"
#include "sh.h"
#include "trap.h"
#include "var.h"

int rootpid;

int main(int argc, char **argv) {
  volatile int state = 0;
  int exception;
  struct stackmark mark;
  struct jmploc jmploc;

  INTOFF;

  if ((exception = setjmp(jmploc.loc))) {
    /* reset the shell */
    unwindredir();
    unwindloops();
    unwindrets();
    closescript();
    yytoken = TNL;
    popstackmark(&mark);
    if (exception == EXINT)
      fputc('\n', stderr);
    switch (state) {
    case 0:
      goto exit;
    case 1:
      goto state1;
    default:
      goto state4;
      break;
    }
  } else {
    pushstackmark(&mark);
    rootpid = getpid();
    initvar();
    signal_init();
  }
  parsefile->isatty = isatty(parsefile->fd);
  handler           = &jmploc;
  FORCEINTON;
  procargs(argc, argv);
  state = 1;
state1:

  if (minusc) {
    evalstring(minusc);
  }

  if (sflag || !minusc)
  state4:
    repl();
exit:
  return exitstatus;
}

int repl() {
  struct cmd *cmd;
  struct stackmark mark;

  for (pushstackmark(&mark); (cmd = parseline()); popstackmark(&mark))
    eval(cmd);

  return exitstatus;
}
