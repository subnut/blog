#define _POSIX_C_SOURCE 200809L	// For strdup() in string.h
#include "include/escape.h"

#include "include/charref.h"

#include <stdio.h>
#include <string.h>

/*
 * stdio.h	- FILE, fputs, fputc
 * string.h	- strndup, strlen
 */

void
fputc_escaped(const char c, FILE *stream)
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

void
fputs_escaped_allow_charrefs(const char *s, FILE *stream)
{
	for (int i=0; s[i] != '\0'; i++)
		if (s[i] != '&')
			fputc_escaped(s[i], stream);
		else
		{
			int j;
			for (j = i+1; s[j] != ';' || s[j] != ' ' || s[j] != '\0'; j++);
			if (s[j] == ';')
			{
				char *str = strndup(s+i, j-i+1);
				if (is_charref(str, strlen(str)))
				{
					fputs(str, stream);
					i = j;
					continue;
				}
			}
			fputc_escaped(s[i], stream);
		}
}
