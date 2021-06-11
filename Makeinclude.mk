.POSIX:
.SUFFIXES:
.SUFFIXES: .c .o

__CFLAGS__	= -Wall -O2 $(CFLAGS)
__CPPFLAGS__	= -I.       $(CPPFLAGS)
__LDFLAGS__	=           $(LDFLAGS)

executables: index blogify htmlize
cleanobj: clean-objects
clean: clean-objects clean-executables
all: executables

clean-objects:
	rm -f src/*.o .htmlize.o
clean-executables:
	rm -f index blogify htmlize

.c.o:
	$(CC) $(__CFLAGS__) $(__CPPFLAGS__) -c "$<" -o "$*.o"

# src/{index,blogify,htmlize}.o depend on constants.h
# i.e. if constant.h changes, re-build them.
src/index.o src/blogify.o src/htmlize.o: constants.h
