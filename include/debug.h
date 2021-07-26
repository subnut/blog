#ifndef DEBUG_H
#define DEBUG_H

#ifdef EBUG
#include <stdio.h>
#define DPUTC(c) fputc(c, stderr)
#define DPUTS(s) fputs(s, stderr)
#define DPRINTF(...) fprintf(stderr, __VA_ARGS__)
#else
#define DPUTC(c)
#define DPUTS(s)
#define DPRINTF(...)
#endif /* EBUG */

#endif /* DEBUG_H */
