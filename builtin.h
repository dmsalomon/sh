/** \file builtin.h
 */

#ifndef BUILTIN_H
#define BUILTIN_H

#include "cmd.h"

typedef int (*builtin_func)(struct cmd*);

builtin_func get_builtin(char *);

#endif
