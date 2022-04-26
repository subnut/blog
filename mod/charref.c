#include "charref.h"
#include "named_charrefs.h"

#include <ctype.h>
#include <stddef.h>
#include <stdbool.h>

/*
 * ctype.h  - isalnum, isalpha
 * stddef.h - NULL
 */

#include "def/streql.h"

bool
is_named_charref(const char *given_str)
{
	/* charrefs MUST start with ampersand */
	if (given_str[0] != '&')
		return false;

	const char *end = strchr(given_str, ';');
	if (end == NULL)	// ie. ';' not found
		return false;

	const char *start = given_str + 1;
	for (int i = 0; i < (sizeof named_charrefs / sizeof named_charrefs[0]); i++)
		if (strneql(start, named_charrefs[i], end - start))
			return true;

	return false;
}

bool
is_charref(const char *given_str)
{
	/* charrefs MUST start with ampersand */
	if (given_str[0] != '&')
		return false;

	const char *end = strchr(given_str, ';');
	if (end == NULL)	// ie. ';' not found
		return false;

	if (given_str[1] == 'x' || given_str[1] == 'X') {
		/* It's a Hexadecimal numeric character reference */
		for (const char *p = &given_str[2]; p < end; p++)
			if (!isxdigit(*p))	// ie. *p isn't xdigit
				return 0;
		return true;
	}
	if (isdigit(given_str[1])) {
		/* It's a Decimal numeric character reference */
		for (const char *p = &given_str[2]; p < end; p++)
			if (!isdigit(*p))	// ie. *p isn't digit
				return false;
		return true;
	}
	if (is_named_charref(given_str))
		return true;

	return false;
}

/* vim:set nowrap fdm=syntax: */
