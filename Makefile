# vim: nowrap

.POSIX:
.SUFFIXES:
.SUFFIXES: .c .o
all:	# default target

BLOG_EXT	= .blog
BLOG_SRCDIR	=  raw
HTML_DESTDIR	=  docs

SRCDIR		=  src
CONFIGURE	= .configure

LDFLAGS 	= -s
CFLAGS		= -Wall -O2

# XXX:	The above CFLAGS aren't POSIX-compliant.
#	Most modern compilers either support or ignore these flags.
#	If yours doesn't, then please change/remove the incompatible CFLAGS

all: makefile executables
debug: clean debug_executables
clean: clean_objects clean_executables
clean_all: clean ; rm -f makefile
clean_objects:
	rm -f */*/*.o

_CFLAGS	= $(CFLAGS) \
	  -D BLOG_EXT=$(BLOG_EXT) \
	  -D BLOG_SRCDIR=$(BLOG_SRCDIR) \
	  -D HTML_DESTDIR=$(HTML_DESTDIR)

.c.o: ; $(CC) $(_CFLAGS:  = ) -c $< -o $*.o -I. -I$(SRCDIR)
# The                 (:  = ) trims multiple spaces into a single space

makefile:
	srcdir=$(SRCDIR) /bin/sh $(CONFIGURE)
	@echo makefile has been updated.
	@echo please run make again.
	@exit 11

makefile: Makefile # makefile is generated from Makefile
makefile: .lib.configure # all of the following configure scripts use it
