.POSIX:
.SUFFIXES:
.SUFFIXES: .c .o
.c.o: ; $(CC) $(_CFLAGS:  = ) -c $< -o $*.o -I .
# The (:  = ) trims multiple spaces to a single space

BLOG_EXT	= .blog
BLOG_SRCDIR	=  raw
HTML_DESTDIR	=  docs

_CFLAGS	= $(CFLAGS) \
	  -D BLOG_EXT=$(BLOG_EXT) \
	  -D BLOG_SRCDIR=$(BLOG_SRCDIR) \
	  -D HTML_DESTDIR=$(HTML_DESTDIR)

# XXX:	These CFLAGS aren't POSIX-compliant.
#	Most modern compilers either support or ignore these flags.
#	If yours doesn't, then please comment out the following line.
CFLAGS	= -Wall -O2
LDFLAGS = -s

all: index blogify htmlize
clean: clean_objects clean_executables

debug: clean all_debug
all_debug: index_debug blogify_debug htmlize_debug
index_debug blogify_debug htmlize_debug:
	@$(MAKE) $(@:_debug=) CFLAGS='-Wall -ggdb' LDFLAGS=

clean_objects:		; rm -f */*.o
clean_executables:	; rm -f index blogify htmlize

index_src	= exe/index.o $(index_deps)
blogify_src	= exe/blogify.o $(blogify_deps)
htmlize_src	= exe/htmlize.o mod/htmlize.o $(htmlize_deps)

index:		$(index_src)
blogify:	$(blogify_src)
htmlize:	$(htmlize_src)

index blogify htmlize:
	@echo $(CC) $(LDFLAGS) -o $@ $$(echo $($@_src) | tr ' ' '\n' | sort | uniq | tr '\n' ' ')
	@     $(CC) $(LDFLAGS) -o $@ $$(echo $($@_src) | tr ' ' '\n' | sort | uniq | tr '\n' ' ')


# Dependencies
exe/blogify.o:	constants.h
exe/blogify.o:	mod/date_to_text.o
exe/blogify.o:	mod/escape.o
exe/blogify.o:	mod/htmlize.o
blogify_deps	= mod/date_to_text.o \
		  mod/escape.o $(escape_deps) \
		  mod/htmlize.o $(htmlize_deps)

mod/charref.o:	named_charrefs.h
charref_deps	= named_charrefs.h

mod/escape.o:	mod/charref.o
escape_deps	= mod/charref.o $(charref_deps)

exe/index.o:	constants.h
exe/index.o:	mod/date_to_text.o
exe/index.o:	mod/escape.o
exe/index.o:	mod/urlencode.o
index_deps	= mod/date_to_text.o \
		  mod/escape.o $(escape_deps) \
		  mod/urlencode.o

mod/htmlize.o:	constants.h
mod/htmlize.o:	mod/escape.o
htmlize_deps	= mod/escape.o $(escape_deps)


# Header files
mod/charref.o:		mod/charref.h
mod/date_to_text.o:	mod/date_to_text.h
mod/escape.o:		mod/escape.h
mod/htmlize.o:		mod/htmlize.h
mod/urlencode.o:	mod/urlencode.h

# vim: nowrap
