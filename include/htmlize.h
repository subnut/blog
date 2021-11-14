#ifndef HTMLIZE_H
#define HTMLIZE_H

int	htmlize(FILE *, FILE *);

#include <stdio.h>
#include <stdbool.h>
#include "constants.h"

/*
 * stdio.h	-	FILE
 * stdbool.h	-	bool
 * constants.h	-	HISTORY_LINES, READAHEAD_LINES
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
	char		*line;	// points to readahead[0]
	char		*history[HISTORY_LINES];
	char		*readahead[READAHEAD_LINES];

#define BUFFER_SIZE ((HISTORY_LINES + READAHEAD_LINES) * MAX_LINE_LENGTH)
	char		 buffer[BUFFER_SIZE];
	char		*bufend; // = buffer + BUFFER_SIZE
};

#endif /* HTMLIZE_H */
