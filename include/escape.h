#ifndef ESCAPE_H
#define ESCAPE_H

#include <stdio.h>	// for FILE

void	fputc_escaped			(const char,	FILE *);
void	fputs_escaped			(const char *,	FILE *);
void	fputs_escaped_allow_charrefs	(const char *,	FILE *);

#endif /* ESCAPE_H */
