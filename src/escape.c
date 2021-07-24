#include <stdio.h>

void
fputc_escaped(char c, FILE *stream)
{
	switch (c)
	{
		case '<': fputs("&lt;",  stream); break;
		case '>': fputs("&gt;",  stream); break;
		case '&': fputs("&amp;", stream); break;
		default:  fputc(c,       stream); break;
	}
}

void
fputs_escaped(const char *s, FILE *stream)
{
	for (int i=0; s[i] != '\0'; i++)
		fputc_escaped(s[i], stream);
}
