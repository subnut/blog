.POSIX:
.SUFFIXES:
.SUFFIXES: .c .o
.c.o: ; $(CC) -Wall -I. $(CFLAGS) -c $< -o $*.o

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
	$(CC) $(LDFLAGS) -o $@ $($@_deps)

# Rebuild these if constants.h is changed
src/index.o src/blogify.o src/htmlize.o: constants.h
