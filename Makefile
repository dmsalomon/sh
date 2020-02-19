
CFLAGS:=-g -Wall
PROG:=sh
DIR:=$(notdir $(basename $(CURDIR)))
TAR:=$(DIR).tar.gz

OBJ=sh.o\
    cmd.o\
    builtin.o\
    output.o\
    lexer.o\
    input.o\
    parser.o\
    mem.o\
    eval.o\
    redir.o\
    error.o\
    trap.o\
    var.o\
    expand.o\

all: $(PROG)

$(PROG): $(OBJ)

clean:
	$(RM) *.o $(PROG) $(TAR)

$(TAR): clean
	cd .. && \
	tar czf $@ $(DIR) && \
	mv $@ $(DIR)/.

dist: $(TAR)

re: clean all

.PHONY: all clean dist re
