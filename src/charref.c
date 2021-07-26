#include <ctype.h>
#include <string.h>

/*
 * ctype.h	- isalnum(), isalpha(), etc.
 * string.h	- str*(), mem*()
 */

#include "include/charref.h"

static const char *named_references[] =
/*
 * Named Character References that will be recognized by htmlize()
 *
 * Say a named Character Reference is "&abcd;", but hasn't been included in
 * this array. htmlize() will turn it to "&nbsp;abcd;". So, the browser shall
 * show "&abcd;" literally instead of the character that was originally
 * referenced by "&abcd;"
 */
{
	"&amp;" ,
	"&nbsp;",
	"&reg;" , "&REG;" ,
	"&copy;", "&COPY;",
	"&mdash", "&ndash",
};

int
is_named_charref(const char *given_str)
/*
 * Note that this function only checks from the beginning of the string. It
 * does not match the whole string.
 *
 * i.e. this function in python would be -
 *	def is_named_charref(given_str):
 *		for i in range(0,len(named_references)):
 *			if given_str.startswith(named_references[i]):
 *				return True
 *		else:
 *			return False
 */
{
	for (int i=0; i < (sizeof(named_references)/sizeof(named_references[0])); i++)
		if (!strncmp(given_str, named_references[i], strlen(named_references[i])))
			return 1;
	return 0;
}

int
is_charref(const char *given_str, size_t n)
/*
 * Checks if the first 'n' bytes of the string pointed to by *given_str
 * contains a valid HTML Character reference at the start of the string.
 */
{
	/* charrefs MUST start with ampersand */
	if (given_str[0] != '&')
		return 1;

	char *end;
	end = memchr(given_str, ';', n);
	if (end == NULL)	// ie. ';' not found
		return 1;

	if (given_str[1] == 'x' || given_str[1] == 'X')
	{
		/* It's a Hexadecimal numeric character reference */
		for (const char *p = &given_str[2]; p < end; p++)
			if (!isxdigit(p))	// ie. *p isn't xdigit
				return 1;
		return 0;
	}
	if (isdigit(given_str[1]))
	{
		/* It's a Decimal numeric character reference */
		for (const char *p = &given_str[2]; p < end; p++)
			if (!isdigit(p))	// ie. *p isn't digit
				return 1;
		return 0;
	}
	if (is_named_charref(given_str))
		return 0;
	return 1;
}

// vim:fdm=syntax:sw=8:sts=8:ts=8:nowrap:
