.POSIX:
.SUFFIXES:
.SUFFIXES: .c .o
.c.o: ; $(CC) -I. $(CFLAGS) -c $< -o $*.o

CFLAGS	= -Wall
LDFLAGS = -s

all: index blogify htmlize
clean: clean_objects clean_executables

debug: clean all_debug
all_debug: index_debug blogify_debug htmlize_debug
index_debug blogify_debug htmlize_debug:
	@$(MAKE) $(@:_debug=) CFLAGS=-ggdb LDFLAGS=

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
src/blogify.o:	src/date_to_text.o	include/date_to_text.h
src/blogify.o:	src/escape.o		include/escape.h
src/blogify.o:	src/htmlize.o		include/htmlize.h
blogify_deps	= src/date_to_text.o \
		  src/escape.o \
		  $(escape_deps) \
		  src/htmlize.o \
		  $(htmlize_deps)

src/escape.o:	src/charref.o		include/charref.h
escape_deps	= src/charref.o

src/index.o:	constants.h
src/index.o:	src/date_to_text.o	include/date_to_text.h
src/index.o:	src/escape.o		include/escape.h
src/index.o:	src/urlencode.o		include/urlencode.h
index_deps	= src/date_to_text.o \
		  src/escape.o \
		  $(escape_deps) \
		  src/urlencode.o

src/htmlize.o:	constants.h
src/htmlize.o:	src/escape.o		include/escape.h
htmlize_deps	= src/escape.o \
		  $(escape_deps)


# Header files
src/charref.o:		include/charref.h
src/date_to_text.0:	include/date_to_text.h
src/escape.o:		include/escape.h
src/htmlize.o:		include/htmlize.h
src/urlencode.o:	include/urlencode.h

# vim: noet ts=8 sts=8 sw=8
