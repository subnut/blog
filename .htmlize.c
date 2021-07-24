#include <stdio.h>
#include "include/htmlize.h"

int
main(void)
{
	int retcode;
	retcode = htmlize(stdin, stdout);
	if (retcode == -1)
		fputs("No lines input\n", stderr);
	return retcode;
}
