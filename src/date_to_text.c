#include "include/date_to_text.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * stdio.h  - perror
 * string.h - strchr
 * stdlib.h - atoi
 */

char *
date_to_text(char *date_str)
{
	char *output;
	char final_str[15];
	output = final_str;

	char *date  = date_str;
	char *month = strchr(date,  '/');
	char *year  = strchr(month, '/');

	/* Date */
	switch (atoi(date))
	{
		case 1:
			memmove(output, "1st", 3);
			output += 3;
			break;
		case 2:
			memmove(output, "2nd", 3);
			output += 3;
			break;
		case 3:
			memmove(output, "3rd", 3);
			output += 3;
			break;

		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
			if (date[0] != '0')	/* eg.  2/../.... */
				*output++ = date[0];
			else				/* eg. 02/../.... */
				*output++ = date[1];
			*output++ = 't';
			*output++ = 'h';
			break;

		default:
			*output++ = date[0];
			*output++ = date[1];
			*output++ = 't';
			*output++ = 'h';
			break;
	}
	*output++ = ' ';

	/* Month */
	switch (atoi(month))
	{
		case 1:
			memmove(output, "Jan", 3);
			output += 3;
			break;
		case 2:
			memmove(output, "Feb", 3);
			output += 3;
			break;
		case 3:
			memmove(output, "Mar", 3);
			output += 3;
			break;
		case 4:
			memmove(output, "Apr", 3);
			output += 3;
			break;
		case 5:
			memmove(output, "May", 3);
			output += 3;
			break;
		case 6:
			memmove(output, "June", 4);
			output += 4;
			break;
		case 7:
			memmove(output, "July", 4);
			output += 4;
			break;
		case 8:
			memmove(output, "Aug", 3);
			output += 3;
			break;
		case 9:
			memmove(output, "Sep", 3);
			output += 3;
			break;
		case 10:
			memmove(output, "Oct", 3);
			output += 3;
			break;
		case 11:
			memmove(output, "Nov", 3);
			output += 3;
			break;
		case 12:
			memmove(output, "Dec", 3);
			output += 3;
			break;
	}
	*output++ = ' ';

	/* Year */
	memmove(output, year, 4);
	output += 4;
	*output++ = '\0';

	if ((date_str = realloc(date_str, 15 * sizeof(char))) == NULL)
		exit((perror("realloc error"), EXIT_FAILURE));

	memmove(date_str, final_str, 15);
	return date_str;
}

// vim:noet:ts=4:sts=0:sw=0:fdm=syntax
