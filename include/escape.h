#ifndef FPUT_ESCAPED_H
#define FPUT_ESCAPED_H

#include <stdio.h>	// for FILE

void	fputc_escaped			(const char,	FILE *);
void	fputs_escaped			(const char *,	FILE *);
void	fputs_escaped_allow_charrefs	(const char *,	FILE *);

#endif /* FPUT_ESCAPED_H */
