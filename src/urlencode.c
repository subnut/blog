#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "include/urlencode.h"

/*
 * ctype.h  - isalnum
 * stdio.h  - sprintf
 * string.h - memmove
 */

char *
urlencode_c(const char c, char *str_size_4)
{
	/* unreserved */
	if (isalnum(c))
	{
		sprintf(str_size_4, "%c", c);
		return str_size_4;
	}
	switch (c)
	{
		case '-':
		case '.':
		case '_':
		case '~':
			sprintf(str_size_4, "%c", c);
			return str_size_4;
		default:
			break;
	}

	/* reserved */
	sprintf(str_size_4, "%%%0X", c);
	return str_size_4;
}

char *
urlencode_s(const char *str, char *storage, size_t len)
{
	/*
	 * const char *str	Pointer to string that shall be converted
	 * const char *storage	Pointer to string for storing the result
	 * size_t len		Size of the array pointed by *storage
	 *
	 * Takes char array *str and stores the result to char array *storage.
	 * Returns the same pointer as *storage for convenience.
	 *
	 * Array size of *storage should ideally be 1 + (3 * (size of *str)),
	 * because, say _every_ character of *str needs to be converted into
	 * the "%XX" format.That means, every character will become 3
	 * characters. So, the string length shall triple. And since we need to
	 * add the trailing '\0', we need an extra character.
	 */

	/* explicit_bzero(storage) */
	for (int i=0; i<len; i++)
		storage[i] = '\0';

	char buffer[4];
	char *pointer;
	pointer = storage;
	for (int i=0; str[i]!='\0' && strlen(storage)<=len; i++)
	{
		urlencode_c(str[i], buffer);
		if (strlen(storage) + strlen(buffer) > len) break;
		memmove(pointer, buffer, strlen(buffer));
		pointer += strlen(buffer);
	}
	return storage;
}


