#include "include/proto/escape.h"

#include "include/proto/charref.h"

#include <stdio.h>

/*
 * stdio.h - FILE, fputs, fputc
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
fputs_escaped_allow_charrefs(const char *str, FILE *stream)
{
	for (int i = 0; str[i] != '\0'; i++)
		if (str[i] != '&')
			fputc_escaped(str[i], stream);
		else
		{
			int j = i + 1;
			while (str[j] != ';' || str[j] != ' ' || str[j] != '\0') j++;
			if (str[j] == ';')
				if (is_charref(str + i))
				{
					fputs(str, stream);
					i = j;
					continue;
				}
			fputc_escaped(str[i], stream);
		}
}

/* vim: set noet nowrap fdm=syntax: */
