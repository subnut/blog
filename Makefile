# vim: nowrap

.POSIX:
.SUFFIXES:
.SUFFIXES: .c .o
all:	# default target

BLOG_EXT	= .blog
BLOG_SRCDIR	=  raw
HTML_DESTDIR	=  docs

# XXX:	These CFLAGS aren't POSIX-compliant.
#	Most modern compilers either support or ignore these flags.
#	If yours doesn't, then please comment out the following line.
CFLAGS	= -Wall -O2
LDFLAGS = -s

all: makefile executables
debug: clean debug_executables
clean: clean_objects
clean_objects:
	rm -f */*.o

_CFLAGS	= $(CFLAGS) \
	  -D BLOG_EXT=$(BLOG_EXT) \
	  -D BLOG_SRCDIR=$(BLOG_SRCDIR) \
	  -D HTML_DESTDIR=$(HTML_DESTDIR)

.c.o: ; $(CC) $(_CFLAGS:  = ) -c $< -o $*.o -I .
# The                 (:  = ) trims multiple spaces into a single space

makefile:
	./configure
	@echo makefile has been updated.
	@echo please run make again.
	@sh -c 'exit 11'

# Dependencies for makefile
makefile: Makefile # makefile is generated from Makefile
