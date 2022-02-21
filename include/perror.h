#ifndef PERROR_R
#define PERROR_R

#include <stdio.h>
#define perror(str) \
	(fprintf(stderr,"%d:%s:%s(): ",__LINE__,__FILE__,__func__), perror(str))

#endif /* PERROR_R */
