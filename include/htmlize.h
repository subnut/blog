#ifndef HTMLIZE_H
#define HTMLIZE_H

#ifndef MAX_LINKS
#define MAX_LINKS 50
#endif

#ifndef MAX_LINE_LENGTH
#define MAX_LINE_LENGTH 500
#endif

#ifndef CONTEXT_LINES
#define CONTEXT_LINES 2
#endif

int	htmlize(FILE *, FILE *);

#include <stdbool.h>
struct config {
	bool BOLD_OPEN;
	bool ITALIC_OPEN;
	bool HTML_TAG_OPEN;
	bool LINK_OPEN;
	bool LINK_TEXT_OPEN;
	bool TABLE_MODE;
	bool LIST_MODE;
};
struct files {
	FILE *src;
	FILE *dest;
};
struct data {
	struct config *config;
	struct files  *files;
	char *line;	// Pointer to the current line
	char lines[CONTEXT_LINES*2 + 1][MAX_LINE_LENGTH];
	size_t nlines;	// Number of lines (expected to be CONTEXT_LINES*2 + 1)
};

#endif /* HTMLIZE_H */
