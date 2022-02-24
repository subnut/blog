#include "include/charref.h"

#include <ctype.h>
#include <string.h>

#define streql(s1, s2) \
	(strcmp(s1, s2) == 0)

/*
 * ctype.h  - isalnum, isalpha
 * string.h - strcmp, strchr
 */

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
 */
{
	static const int max = (int)(sizeof named_references / sizeof named_references[0]);
	for (int i = 0; i < max; i++)
		if (streql(given_str, named_references[i]))
			return 0;
	return 1;
}

int
is_charref(const char *given_str)
{
	/* charrefs MUST start with ampersand */
	if (given_str[0] != '&')
		return 0;

	char *end;
	end = strchr(given_str, ';');
	if (end == NULL)	// ie. ';' not found
		return 0;

	if (given_str[1] == 'x' || given_str[1] == 'X')
	{
		/* It's a Hexadecimal numeric character reference */
		for (const char *p = &given_str[2]; p < end; p++)
			if (!isxdigit(*p))	// ie. *p isn't xdigit
				return 0;
		return 1;
	}
	if (isdigit(given_str[1]))
	{
		/* It's a Decimal numeric character reference */
		for (const char *p = &given_str[2]; p < end; p++)
			if (!isdigit(*p))	// ie. *p isn't digit
				return 0;
		return 1;
	}
	if (!is_named_charref(given_str))
		return 1;
	return 1;
}

/* vim: set noet nowrap fdm=syntax: */
