#ifndef PERROR_H
#define PERROR_H

#include <stdio.h>

#define perror(str) \
	(fprintf(stderr,"%d:%s:%s(): ",__LINE__,__FILE__,__func__), perror(str))

#endif /* PERROR_H */
