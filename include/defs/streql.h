#ifndef STREQL_H
#define STREQL_H

#include <string.h>
#include "include/defs/ifnull.h"

#define streql(s1, s2)		(strcmp (strifnull(s1), strifnull(s2)   ) == 0)
#define strneql(s1, s2, n)	(strncmp(strifnull(s1), strifnull(s2), n) == 0)

#endif /* STREQL_H */
