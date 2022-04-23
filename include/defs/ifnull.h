#ifndef IFNULL_H
#define IFNULL_H

#include <stddef.h>

/*
 * stddef.h - NULL
 */

#define ifnull(ptr, val)	(ptr == NULL ? val : ptr)
#define strifnull(str)		(ifnull(str, "NULL"))

#endif /* IFNULL_H */
