.POSIX:
.SUFFIXES:
.SUFFIXES: .c .o

__CFLAGS__	= -Wall -O2 $(CFLAGS)
__CPPFLAGS__	= -I.       $(CPPFLAGS)
__LDFLAGS__	=           $(LDFLAGS)

executables: index blogify htmlize
cleanobj: clean-objects
clean: clean-objects clean-executables


clean-objects:
	rm -f src/*.o .htmlize.o
clean-executables:
	rm -f index blogify htmlize


# src/{index,blogify,htmlize}.o depend on constants.h
# i.e. if constant.h changes, re-build them.
src/index.o src/blogify.o src/htmlize.o: constants.h

.c.o:
	$(CC) $(__CFLAGS__) $(__CPPFLAGS__) -c "$<" -o "$*.o"


# ------------------------------------------------------------------------------
index:		src/index.o   src/cd.o src/date_to_text.o src/stoi.o
	$(CC) $(__LDFLAGS__) -o $@ \
		src/index.o   src/cd.o src/date_to_text.o src/stoi.o

blogify:	src/blogify.o src/cd.o src/date_to_text.o src/stoi.o src/htmlize.o
	$(CC) $(__LDFLAGS__) -o $@ \
		src/blogify.o src/cd.o src/date_to_text.o src/stoi.o src/htmlize.o

htmlize:	.htmlize.o             src/date_to_text.o src/stoi.o src/htmlize.o
	$(CC) $(__LDFLAGS__) -o $@ \
		.htmlize.o             src/date_to_text.o src/stoi.o src/htmlize.o
