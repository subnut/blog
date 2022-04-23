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
		return 0;

	const char *end = strchr(given_str, ';');
	if (end == NULL)	// ie. ';' not found
		return 0;

	const char *start = given_str + 1;
	for (int i = 0; i < (sizeof named_charrefs / sizeof named_charrefs[0]); i++)
		if (strneql(start, named_charrefs[i], end - start))
			return 1;

	return 0;
}

bool
is_charref(const char *given_str)
{
	/* charrefs MUST start with ampersand */
	if (given_str[0] != '&')
		return 0;

	const char *end = strchr(given_str, ';');
	if (end == NULL)	// ie. ';' not found
		return 0;

	if (given_str[1] == 'x' || given_str[1] == 'X') {
		/* It's a Hexadecimal numeric character reference */
		for (const char *p = &given_str[2]; p < end; p++)
			if (!isxdigit(*p))	// ie. *p isn't xdigit
				return 0;
		return 1;
	}
	if (isdigit(given_str[1])) {
		/* It's a Decimal numeric character reference */
		for (const char *p = &given_str[2]; p < end; p++)
			if (!isdigit(*p))	// ie. *p isn't digit
				return 0;
		return 1;
	}
	if (is_named_charref(given_str))
		return 1;

	return 0;
}

/* vim:set nowrap fdm=syntax: */
