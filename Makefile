.POSIX:
.SUFFIXES:
.SUFFIXES: .c .o
.c.o: ; $(CC) $(__CFLAGS__) $(__CPPFLAGS__) -c "$<" -o "$*.o"

__CFLAGS__    = -Wall -O2 $(CFLAGS)
__CPPFLAGS__  = -I. $(CPPFLAGS)
__LDFLAGS__   = $(LDFLAGS)

index-deps    =  src/index.o    src/cd.o src/date_to_text.o src/stoi.o
blogify-deps  =  src/blogify.o  src/cd.o src/date_to_text.o src/stoi.o src/htmlize.o
htmlize-deps  =  .htmlize.o              src/date_to_text.o src/stoi.o src/htmlize.o

all: index blogify htmlize
clean: clean-objects clean-executables

clean-objects:     ; rm -f src/*.o .htmlize.o
clean-executables: ; rm -f index blogify htmlize

index:   $(index-deps)
blogify: $(blogify-deps)
htmlize: $(htmlize-deps)

index blogify htmlize:
	 $(CC) $(__LDFLAGS__) -o $@ $($@-deps)

# Rebuild these if constants.h is changed
src/index.o src/blogify.o src/htmlize.o: constants.h
