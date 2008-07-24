#CFLAGS+=-O -Wall -ansi -g -pedantic
CC=gcc
APXS?=`which apxs`

all: mod_pspbrowse.lo

test: pspbrowse.o test.o
	$(CC) -o $(.TARGET) $(.ALLSRC)

mod_pspbrowse.lo: mod_pspbrowse.c pspbrowse.c
	$(APXS) -c $(.ALLSRC)

install: mod_pspbrowse.c pspbrowse.c
	$(APXS) -c -i $(.ALLSRC)

activate: mod_pspbrowse.c pspbrowse.c
	$(APXS) -c -i -a $(.ALLSRC)

clean:
	@rm -f *.o *.so *.slo *.lo *.la
	@rm -Rf .libs

