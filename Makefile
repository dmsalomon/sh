
CFLAGS:=-Wall -Wextra -ggdb
PROG:=sh
DIR:=$(notdir $(basename $(CURDIR)))
TAR:=$(DIR).tar.gz

SRC:=$(wildcard *.c)
OBJ:=$(patsubst %.c,%.o,$(SRC))
DEP:=$(patsubst %.c,%.d,$(SRC))

all: $(PROG)

$(PROG): $(OBJ)

-include $(DEP)

%.o: %.c Makefile
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

.PHONY: clean
clean:
	$(RM) *.d *.o $(PROG) $(TAR) compile_commands.json

$(TAR): clean
	cd .. && \
	tar --exclude-vcs -czf $@ $(DIR) && \
	mv $@ $(DIR)/.

.PHONY: dist
dist: $(TAR)

.PHONY: re
re: clean all

debug: CFLAGS += -ggdb -DDEBUG
debug: all

.PHONY: fmt
fmt:
	clang-format -i *.c *.h

.PHONY: run
run: all
	rlwrap -cpPurple ./sh
