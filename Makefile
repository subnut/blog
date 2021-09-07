.POSIX:
.SUFFIXES:
.SUFFIXES: .c .o
.c.o: ; $(CC) $(CFLAGS__) $(CPPFLAGS__) -c "$<" -o "$*.o"

CC            = c99
CFLAGS__      = -Wall -O1 $(CFLAGS)
CPPFLAGS__    = -I. $(CPPFLAGS)
LDFLAGS__     = $(LDFLAGS)

index_deps    =  src/index.o    src/cd.o src/date_to_text.o src/stoi.o src/escape.o src/urlencode.o
blogify_deps  =  src/blogify.o  src/cd.o src/date_to_text.o src/stoi.o src/escape.o src/urlencode.o src/charref.o src/htmlize.o
htmlize_deps  =  .htmlize.o              src/date_to_text.o src/stoi.o src/escape.o src/urlencode.o src/charref.o src/htmlize.o

all: index blogify htmlize
clean: clean_objects clean_executables

clean_objects:     ; rm -f src/*.o .htmlize.o
clean_executables: ; rm -f index blogify htmlize

index:   $(index_deps)
blogify: $(blogify_deps)
htmlize: $(htmlize_deps)

index blogify htmlize:
	$(CC) $(LDFLAGS__) -o $@ $($@_deps)

# Rebuild these if constants.h is changed
src/index.o src/blogify.o src/htmlize.o: constants.h


# vim:nowrap:
