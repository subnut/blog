.POSIX:
.SUFFIXES:
.SUFFIXES: .c .o
.c.o: ; $(CC) -Wall -I. $(CFLAGS) -c $< -o $*.o

all: index blogify htmlize
clean: clean_objects clean_executables

clean_objects:     ; rm -f src/*.o .htmlize.o
clean_executables: ; rm -f index blogify htmlize

htmlize_deps	= src/htmlize.o src/charref.o src/escape.o
index_sources   = src/index.o src/date_to_text.o src/escape.o src/urlencode.o
blogify_sources = $(htmlize_deps) src/blogify.o src/date_to_text.o
htmlize_sources = $(htmlize_deps) .htmlize.o

index:   $(index_sources)
blogify: $(blogify_sources)
htmlize: $(htmlize_sources)

index blogify htmlize:
	$(CC) $(LDFLAGS) -o $@ $($@_sources)

# Rebuild these if constants.h has changed
src/index.o:   constants.h
src/blogify.o: constants.h
src/htmlize.o: constants.h
