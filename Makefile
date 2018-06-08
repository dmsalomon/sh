
CFLAGS:=-g -Wall
PROG:=sh
ZIP:=hw4_dms833.zip

OBJ=sh.o\
    cmd.o\
    builtin.o\
    util.o\

all: $(PROG)

$(PROG): $(OBJ)

clean:
	$(RM) *.o $(PROG) $(ZIP)

$(ZIP): clean
	cd .. && \
	zip -r $@ hw4/*.[ch] hw4/Makefile && \
	mv $@ hw4/.

.PHONY: all clean upload
