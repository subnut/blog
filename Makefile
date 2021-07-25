# vim:nowrap:

.POSIX:
.SUFFIXES:
.SUFFIXES: .c .o
.c.o: ; $(CC) $(__CFLAGS__) $(__CPPFLAGS__) -c "$<" -o "$*.o"

__CFLAGS__    = -Wall -O2 $(CFLAGS)
__CPPFLAGS__  = -I. $(CPPFLAGS)
__LDFLAGS__   = $(LDFLAGS)

index_deps    =  src/index.o    src/cd.o src/date_to_text.o src/stoi.o src/escape.o src/urlencode.o
blogify_deps  =  src/blogify.o  src/cd.o src/date_to_text.o src/stoi.o src/escape.o src/urlencode.o src/htmlize.o
htmlize_deps  =  .htmlize.o              src/date_to_text.o src/stoi.o src/escape.o src/urlencode.o src/htmlize.o

all: index blogify htmlize
clean: clean_objects clean_executables

clean_objects:     ; rm -f src/*.o .htmlize.o
clean_executables: ; rm -f index blogify htmlize

index:   $(index_deps)
blogify: $(blogify_deps)
htmlize: $(htmlize_deps)

index blogify htmlize:
	$(CC) $(__LDFLAGS__) -o $@ $($@_deps)

# Rebuild these if constants.h is changed
src/index.o src/blogify.o src/htmlize.o: constants.h
