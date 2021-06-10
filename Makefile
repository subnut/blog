.POSIX:
.SUFFIXES: .c .o

__CFLAGS__	= -Wall -O2 $(CFLAGS)
__CPPFLAGS__	= -I.       $(CPPFLAGS)
__LDFLAGS__	=           $(LDFLAGS)

make:
	@ln -sf src/* . 2>/dev/null
	@sh -c 'for FILE in *.c; do mv "$$FILE" ".$$FILE"; done'
	@exec $(MAKE) all
	@rm -f .*.c

clean:
	rm -f .*.c .*.o index blogify htmlize

all: index blogify htmlize


index.o blogify.o htmlize.o: constants.h
.c.o:
	$(CC) $(__CFLAGS__) $(__CPPFLAGS__) -c "$<"


index:   .index.o   .cd.o .date_to_text.o .stoi.o
	$(CC)   $(__LDFLAGS__) -o $@ .index.o   .cd.o .date_to_text.o .stoi.o

blogify: .blogify.o .cd.o .date_to_text.o .stoi.o .htmlize.o
	$(CC)   $(__LDFLAGS__) -o $@ .blogify.o .cd.o .date_to_text.o .stoi.o .htmlize.o

htmlize: .date_to_text.o .stoi.o .htmlize.o
	@echo '#include <stdio.h>' "\n" \
	'#include "include/htmlize.h"' "\n" \
	'int main(void) { htmlize(stdin, stdout); return 0; }' |\
	$(CC) $(__CFLAGS__) $(__CPPFLAGS__) \
		$(__LDFLAGS__) -o $@                .date_to_text.o .stoi.o .htmlize.o \
		-x c -

