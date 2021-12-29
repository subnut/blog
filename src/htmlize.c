#define _POSIX_C_SOURCE 200809L
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * _POSIX_C_SOURCE  - getline
 * stdbool.h        - bool, true, false
 * stdio.h          - getline, FILE
 * stdlib.h			- free, EXIT_SUCCESS, EXIT_FAILURE
 * string.h			- strncmp
 */

#include "constants.h"
#include "include/htmlize.h"

struct config {
	bool bolded;
	bool italicized;
};
struct lines {
	char *curline;	// Should be set at readahead[0] before every pass
	char *history	[HISTORY_LINES];
	char *readahead	[READAHEAD_LINES];
};
struct files {
	FILE *in;
	FILE *out;
};
struct data {
	struct config *config;
	struct lines  *lines;
	struct files  *files;
};

int
get_next_line(struct data *data)
{
	/* Structure of data -
	 *	readahead[READAHEAD_LINES-1]	<-- input
	 * 	readahead[READAHEAD_LINES-2]
	 * 	readahead[READAHEAD_LINES-3]
	 * 	...
	 * 	readahead[1]
	 * 	readahead[0]
	 * 	history[0]
	 * 	history[1]
	 * 	history[2]
	 * 	...
	 * 	history[HISTORY_LINES-2]
	 * 	history[HISTORY_LINES-1]	--> free at next iteration
	 */

	/* Shift lines */
	free(data->lines->history[HISTORY_LINES-1]);
	for (int i = HISTORY_LINES-1; i>0; i--)
		data->lines->history[i] = data->lines->history[i-1];

	data->lines->history[0] = data->lines->readahead[0];
	for (int i=1; i < READAHEAD_LINES; i++)
		data->lines->readahead[i-1] = data->lines->readahead[i];

	/*
	 * Avoid the situation where we read more than we need.
	 *
	 * If we have already read "---\n", set new line to NULL and return.
	 * If any of the previous lines is NULL (ie. we had reached EOF or "---\n")
	 * then set new line to NULL and return.
	 */
	for (int i = READAHEAD_LINES-1; i>0; i--)
		if (data->lines->readahead[i] == NULL || strncmp(data->lines->readahead[i], "---\n", 4) == 0)
			return (data->lines->readahead[READAHEAD_LINES-1] = NULL),
				   EXIT_SUCCESS;

	/* Read next line */
	static size_t _ = 0;
	data->lines->readahead[READAHEAD_LINES-1] = NULL;
	if (getline(&data->lines->readahead[READAHEAD_LINES-1], &_, data->files->in) == -1)
	{
		if (fgetc(data->files->in) == EOF)
			/*
			 * We have reached EOF. Set new line to NULL to indicate this fact
			 * to future invocations
			 */
			data->lines->readahead[READAHEAD_LINES-1] = NULL;
		else
			/*
			 * We haven't reached EOF. So, getline errored out due to a
			 * different (possibly dangerous) reason. Use perror to inform the
			 * user about this error, and return EXIT_FAILURE
			 *
			 * NOTE: It is OK to use perror here, because fgetc doesn't clobber
			 * errno. So, the errno must have been set by getline.
			 */
			return perror("getline error"),
				   EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

/*
 * Functions shall be of the form -
 * 	int name(struct data *)
 *
 * Functions shall return 0 if it did nothing, and 1 if it did something.
 * So,
 * 	for (i=0;something;i++)
 * 		if ((functions[i])(&data))
 * 			continue;
 */

int
htmlize(FILE *src, FILE *dest)
{
	/* Initialize structs */
	struct config config;
	struct lines lines;
	struct files files;
	struct data data;

	/* Populate config */
	config.bolded = false;
	config.italicized = false;

	/* Populate lines */
	for (int i = 0; i < HISTORY_LINES; i++) lines.history[i] = NULL;
	for (int i = 0; i < READAHEAD_LINES; i++) lines.readahead[i] = NULL;

	/* Populate files */
	files.out = dest;
	files.in = src;

	/* Populate data */
	data.config = &config;
	data.lines = &lines;
	data.files = &files;

	/* Populate lines.readahead[] */
	static size_t _;
	for (int i = 0; i < READAHEAD_LINES; i++)
	{
		_ = 0;
		if (getline(&lines.readahead[i], &_, files.in) == -1)
			if (fgetc(files.in) != EOF)
				return perror("getline error"),
					   EXIT_FAILURE;
			else break;
	}


	// XXX: temp, for testing.
	lines.curline = lines.readahead[0];
	while (lines.curline != NULL)
	{
		puts("printf");
		printf("%p: %s", lines.curline, lines.curline);
		puts("get_next_line");
		get_next_line(&data);
		lines.curline = lines.readahead[0];
	}

	return EXIT_SUCCESS;
}

/*
 * SYNTAX -
 * 	*bold*
 * 	_italic_
 */

/* vim:set noet nowrap ts=4 sts=4 sw=4 fdm=syntax: */
