#ifndef FREE_H
#define FREE_H

#include <stdlib.h>

#define free(ptr)	{ free(ptr); ptr = NULL; }

#endif /* FREE_H */
