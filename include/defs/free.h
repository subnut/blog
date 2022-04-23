#ifndef FREE_H
#define FREE_H

#include <stddef.h>
#include <stdlib.h>

/*
 * stddef.h - NULL
 */

#define free(ptr)	{ free(ptr); ptr = NULL; }

#endif /* FREE_H */
