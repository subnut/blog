.POSIX:
.SUFFIXES:
.SUFFIXES: .c .o
.c.o: ; $(CC) $(CFLAGS) -c $< -o $*.o -I .

# XXX: The line following this comment isn't POSIX-compliant.
#      Most modern compilers either support or ignore these flags.
#      If yours doesn't, then please comment out the following line.
#      (It only exists for developer convenience)
CFLAGS = -Wall -O2

# Strip executables by default
LDFLAGS = -s

all: index blogify htmlize
clean: clean_objects clean_executables

debug: clean all_debug
all_debug: index_debug blogify_debug htmlize_debug
index_debug blogify_debug htmlize_debug:
	@$(MAKE) $(@:_debug=) CFLAGS='-Wall -ggdb' LDFLAGS=

clean_objects:		; rm -f src/*.o .htmlize.o
clean_executables:	; rm -f index blogify htmlize

index_src	= src/index.o $(index_deps)
blogify_src	= src/blogify.o $(blogify_deps)
htmlize_src	= .htmlize.o src/htmlize.o $(htmlize_deps)

index:		$(index_src)
blogify:	$(blogify_src)
htmlize:	$(htmlize_src)

index blogify htmlize:
	@echo $(CC) $(LDFLAGS) -o $@ $$(echo $($@_src) | tr ' ' '\n' | sort | uniq | tr '\n' ' ')
	@     $(CC) $(LDFLAGS) -o $@ $$(echo $($@_src) | tr ' ' '\n' | sort | uniq | tr '\n' ' ')


# Dependencies
src/blogify.o:	constants.h
src/blogify.o:	src/date_to_text.o
src/blogify.o:	src/escape.o
src/blogify.o:	src/htmlize.o
blogify_deps	= src/date_to_text.o \
		  src/escape.o $(escape_deps) \
		  src/htmlize.o $(htmlize_deps)

src/charref.o:	named_charrefs.h
charref_deps	= named_charrefs.h

src/escape.o:	src/charref.o
escape_deps	= src/charref.o $(charref_deps)

src/index.o:	constants.h
src/index.o:	src/date_to_text.o
src/index.o:	src/escape.o
src/index.o:	src/urlencode.o
index_deps	= src/date_to_text.o \
		  src/escape.o $(escape_deps) \
		  src/urlencode.o

src/htmlize.o:	constants.h
src/htmlize.o:	src/escape.o
htmlize_deps	= src/escape.o $(escape_deps)


# Header files
src/charref.o:		include/proto/charref.h
src/date_to_text.o:	include/proto/date_to_text.h
src/escape.o:		include/proto/escape.h
src/htmlize.o:		include/proto/htmlize.h
src/urlencode.o:	include/proto/urlencode.h

# vim: noet ts=8 sts=8 sw=8
