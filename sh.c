/* \file sh.c
 *
 * A simple shell
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmd.h"
#include "error.h"
#include "eval.h"
#include "input.h"
#include "lexer.h"
#include "mem.h"
#include "output.h"
#include "parser.h"
#include "redir.h"
#include "sh.h"
#include "trap.h"
#include "var.h"

int rootpid;

int main(int argc, char **argv) {
  int fd, exception, status;
  struct stackmark mark;
  struct jmploc jmploc;

  if (argc > 1) {
    if (strcmp(argv[1], "-c") == 0) {
      if (argc > 2)
        setinputstring(argv[2], 0);
      else
        die("-c requires an argument");
    } else {
      fd = setinputfile(argv[1], INPUT_NOFILE_OK);
      if (fd < 0)
        sdie(127, "%s:", argv[1]);
    }
  }

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
    status = exitstatus;
  } else {
    pushstackmark(&mark);
    rootpid = getpid();
    initvar();
    signal_init();
    status = 0;
  }
  handler = &jmploc;
  FORCEINTON;

  return repl(status);
}

int repl(int status) {
  struct cmd *cmd;
  struct stackmark mark;

  for (pushstackmark(&mark); (cmd = parseline()); popstackmark(&mark))
    status = eval(cmd);

  return status;
}
