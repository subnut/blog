#ifndef HTMLIZE_H
#define HTMLIZE_H

int	htmlize(FILE *, FILE *);

#include <stdio.h>
#include <stdbool.h>
#include "constants.h"

/*
 * stdio.h	-	FILE
 * stdbool.h	-	bool
 * constants.h	-	HISTORY, READAHEAD
 */

struct config {
	bool BOLD_OPEN;
	bool ITALIC_OPEN;
	bool LINK_OPEN;
	bool TABLE_MODE;
};

struct files {
	FILE *src;
	FILE *dest;
};

struct data {
	struct config	*config;
	struct files	*files;
	char		*line;
	char		 history[HISTORY][MAX_LINE_LENGTH];
	char		 readahead[READAHEAD][MAX_LINE_LENGTH];
};

#endif /* HTMLIZE_H */
