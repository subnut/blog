#define _POSIX_C_SOURCE 200809L
#include "include/htmlize.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * _POSIX_C_SOURCE  - getline, strndup
 * ctype.h			- isdigit, isspace
 * stdbool.h        - bool, true, false
 * stdio.h          - getline, FILE
 * stdlib.h			- free, atoi, exit, EXIT_SUCCESS, EXIT_FAILURE
 * string.h			- strncmp, strchr, strrchr
 */

#include "constants.h"
#include "include/escape.h"

#define streql(s1, s2) (strcmp(s1, s2) == 0)
#define strneql(s1, s2, n) (strncmp(s1, s2, n) == 0)

/* Struct definitions */
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
	struct links  *links;
};
struct links {
	int maxindex;				// Also see src/index.c
	struct linkdef **linkdefs;	// Array of pointers to struct linkdef
};
struct linkdef {
	enum { LINK, LINKARRAY } type;
	union {
		char *link;
		struct links *links;
	};
};

/* Function prototypes */
static void get_next_line			(struct data *);
static void free_links_recursively	(struct data *);
static int _LINKDEF					(struct data *);
static int _PREFORMATTED			(struct data *);
static int _LINK					(struct data *);

static int (*LINEWISE_FUNCTIONS[])(struct data *) = {
	&_PREFORMATTED,
	&_LINKDEF,
};

static void
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
		if (data->lines->readahead[i] == NULL || strneql(data->lines->readahead[i], "---\n", 4))
		{
			data->lines->readahead[READAHEAD_LINES-1] = NULL;
			goto success;
		}

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
			 * user about this error, and exit(EXIT_FAILURE)
			 *
			 * NOTE: It is OK to use perror here, because fgetc doesn't clobber
			 * errno. So, the errno must have been set by getline.
			 */
			exit((perror("getline error"), EXIT_FAILURE));
	}

success:
	/* Reset lines.curline to lines.readahead[0] */
	data->lines->curline = data->lines->readahead[0];
}

static void
free_links_recursively(struct data *data)
#define free(ptr) { free(ptr); ptr = NULL; }
{
	for (int index = 0; index <= data->links->maxindex; index++)
	{
		if (data->links->linkdefs[index] == NULL)
			continue;

		if (data->links->linkdefs[index]->type == LINK)
			free(data->links->linkdefs[index]->link)
		else /* data->links->linkdefs[index]->type == LINK */
			for (int subindex = 0; subindex <= data->links->linkdefs[index]->links->maxindex; subindex++)
			{
				if (data->links->linkdefs[index]->links->linkdefs[subindex] == NULL) continue;
				free(data->links->linkdefs[index]->links->linkdefs[subindex]->link)
				free(data->links->linkdefs[index]->links->linkdefs[subindex])
			}
		free(data->links->linkdefs[index])
	}
	free(data->links->linkdefs)
}
#undef free

/*
 * Functions shall be of the form -
 * 	static int _name(struct data *)
 *
 * Functions shall return 0 if it did nothing, and 1 if it did something.
 * So,
 * 	for (i=0;something;i++)
 * 		if ((functions[i])(&data))
 * 			continue;
 */

static int
_LINKDEF(struct data *data)
{
	/*
	 * Linkdefs shall be of the form:
	 *
	 * |
	 * |	[1]:   some-link
	 * |	[2.1]: some-other-link
	 * |	[2.2]:    mis-aligned-link
	 * |	[2.10]:hey-im-a-link
	 * |
	 *
	 * Only [1.1] and [1] are valid link IDs. [1.1.1] is invalid.
	 * If [1] is defined, [1.1] should not be defined, and vice-versa. (TODO: error out)
	 * A link defined once can be used only once. After that, it is undefined.
	 *
	 * So, if we define [1], and then we use [1] somewhere, and then we define
	 * [1.1], it is valid. No errors shall be shown.
	 */

	// Ephermal variables
	char *end;
	char *decimal;
	int index;
	int subindex;

	// Valuable variables
	char *id;
	char *href;

#define curline data->lines->curline

	/* Check if current line is a valid linkdef */
	if (curline[0] != '\t')		return 0;
	if (curline[1] != '[')		return 0;
	if (!isdigit(curline[2]))	return 0;

	/* Extract href and strdup it */
	if ((href = strchr(curline, ':')) == NULL) return 0;
	while (isspace(href[0])) href++;
	if (href[0] == '\0') return 0;	// EOL reached! ie. No link given, only id!
	end = strrchr(curline, '\0');
	while (end > href && isspace(end[-1])) end--;
	if ((href = strndup(href, end - href)) == NULL)
		exit((perror("strndup failed"), EXIT_FAILURE));

	/* Extract id */
	id = curline + 2;	// \t[
	end = strchr(id, ']');
	if (end[-1] == '.')	// [1.] is not allowed, only [1] or [1.1]
		goto nope;
	for (char *c = id; c < end; c++)
		if (*c != '.' && !isdigit(*c))	// Only numbers and '.' allowed
			goto nope;
	if (strchr(id, '.') != NULL)
		if (strchr(id, '.') != strrchr(id, '.'))	// Only one '.' allowed
			goto nope;

#undef curline

	/* Convert id from string to integer.
	 * Separate id into index and subindex, if applicable */
	index = atoi(id);
	decimal = strchr(id, '.');	// NULL if no decimal point

	/* allocate enough space for data->links->linkdefs[index] */
	if (data->links->maxindex < index)
	{
		if (realloc(data->links->linkdefs, (index+1) * sizeof(struct linkdef *)) == NULL)
			exit((perror("realloc failed"), EXIT_FAILURE));
		for (int i = data->links->maxindex + 1; i <= index; i++)
			data->links->linkdefs[i] = NULL;
		data->links->maxindex = index;
	}

	/* If there is no subindex */
	if (decimal == NULL)
	{
		if (data->links->linkdefs[index] != NULL)
			exit((fprintf(stderr, data->links->linkdefs[index]->type == LINK
									? "link [%d] already defined\n"
									: "link [%d.*] already defined\n"
							, index), EXIT_FAILURE));

		if ((data->links->linkdefs[index] = calloc(1, sizeof(struct linkdef))) == NULL)
			exit((perror("calloc failed"), EXIT_FAILURE));

		data->links->linkdefs[index]->type = LINK;
		data->links->linkdefs[index]->link = href;

		goto done;
	}

	/* If there is a subindex */
	if (decimal != NULL)
	{
		subindex = atoi(decimal+1);	// +1 because atoi needs pointer to start of integer

		/* Check if we have been scammed */
		if (data->links->linkdefs[index] != NULL)
		{
			if (data->links->linkdefs[index]->type == LINK)
				exit((fprintf(stderr, "link [%d] already defined, can't define [%d.%d]\n",
								index, index, subindex), EXIT_FAILURE));

			if (subindex < data->links->linkdefs[index]->links->maxindex
					&& data->links->linkdefs[index]->links->linkdefs[subindex] != NULL)
				exit((fprintf(stderr, "link [%d.%d] already defined\n",
								index, subindex), EXIT_FAILURE));
		}

		/* Ensure enough space for all of us */
		if (data->links->linkdefs[index] == NULL)
		{
			if ((data->links->linkdefs[index] = calloc(1, sizeof(struct linkdef))) == NULL)
				exit((perror("calloc failed"), EXIT_FAILURE));
			data->links->linkdefs[index]->type = LINKARRAY;
			if ((data->links->linkdefs[index]->links = malloc(sizeof(struct links))) == NULL)
				exit((perror("malloc failed"), EXIT_FAILURE));
			data->links->linkdefs[index]->links->maxindex = subindex;
			for (int i = 0; i <= subindex; i++)
				data->links->linkdefs[index]->links->linkdefs[i] = NULL;
		}
		else /* data->links->linkdefs[index] != NULL */
		{
			if (data->links->linkdefs[index]->links->maxindex < subindex)
				if (realloc(data->links->linkdefs[index]->links->linkdefs,
							(subindex+1) * sizeof(struct linkdef *)) == NULL)
					exit((perror("realloc failed"), EXIT_FAILURE));
			for (int i = data->links->linkdefs[index]->links->maxindex + 1; i <= subindex; i++)
				data->links->linkdefs[index]->links->linkdefs[i] = NULL;
			data->links->linkdefs[index]->links->maxindex = subindex;
		}
		data->links->linkdefs[index]->links->linkdefs[subindex]->type = LINK;
		data->links->linkdefs[index]->links->linkdefs[subindex]->link = href;

		goto done;
	}

done:
	return 1;

nope:
	free(href);
	return 0;
}

static int
_PREFORMATTED(struct data *data)
#define curline data->lines->curline
{
	/*
	 * ```			=> <pre> ... </pre>
	 * ```samp		=> <pre><samp> ... </code><samp>
	 * ```code html	=> <pre><code class="language-html"> ... </code><pre>
	 */

	if (!strneql(curline, "```", 3))
		return 0;

	curline += 3;	// For "```"
	fputs("<pre>", data->files->out);

	static enum {
		NONE,
		SAMP,
		CODE
	} modifier = NONE;

	if (!isspace(*curline))
	{
		if (streql(curline, "samp\n"))
		{
			fputs("<samp>", data->files->out);
			modifier = SAMP;
		}
		else if (strneql(curline, "code", 4))
		{
			curline += 4;	// For "code"
			modifier = CODE;
			if (*curline != '\n')
			{
				/* Skip over all whitespaces */
				while (isspace(*curline))
					curline++;

				char *p;
				*(p = strchr(curline, '\n')) = '\0';
				fprintf(data->files->out, "<code class=\"lanugage-%s\">", curline);
				*p = '\n';
			}
			else
				fputs("<code>", data->files->out);
		}
	}

	while (get_next_line(data), curline != NULL && !streql(curline, "```\n"))
		fputs_escaped(curline, data->files->out);

	switch (modifier)
	{
		case CODE:
			fputs("</code>", data->files->out);
			break;
		case SAMP:
			fputs("</samp>", data->files->out);
			break;
		case NONE:
			break;
	}
	fputs("</pre>\n", data->files->out);

	if (streql(curline, "```\n"))
		get_next_line(data);

	return 1;
}
#undef curline

int
htmlize(FILE *src, FILE *dest)
{
	/* Initialize structs */
	struct config config;
	struct lines lines;
	struct files files;
	struct links links;
	struct data data;

	/* Populate links */
	if ((links.linkdefs = calloc(1, sizeof(struct linkdef *))) == NULL)
		return perror("calloc error"),
			   EXIT_FAILURE;
	links.maxindex = 0;
	links.linkdefs[0] = NULL;

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
	data.links = &links;

	/* Populate lines.readahead[] */
	static size_t _;
	for (int i = 0; i < READAHEAD_LINES; i++)
	{
		_ = 0;
		if (getline(&lines.readahead[i], &_, files.in) == -1)
		{
			if (fgetc(files.in) != EOF)
				return perror("getline error"),
					   EXIT_FAILURE;
			else
				/* We've reached EOF, so break. */
				break;
		}
		if (strneql(lines.readahead[i], "---\n", 4))
		{
			/*
			 * We have reached the "---\n" that marks the end of our parsing.
			 * Don't proceed further.
			 */
			for (i++; i < READAHEAD_LINES; i++)
				lines.readahead[i] = NULL;	// Mark the remaining lines as NULL
			break;
		}
	}
	lines.curline = lines.readahead[0];

	/* Iterate over the lines */
	for (; lines.curline != NULL; get_next_line(&data))
	{
		for (int i = 0; i < (sizeof(LINEWISE_FUNCTIONS)/sizeof(LINEWISE_FUNCTIONS[0])); i++)
			if ((LINEWISE_FUNCTIONS[i])(&data))
				continue;
		fputs(lines.curline, files.out);
	}

	free_links_recursively(&data);
	return EXIT_SUCCESS;
}

/*
 * SYNTAX -
 * 	*bold*
 * 	_italic_
 */

/* vim:set noet nowrap ts=4 sts=4 sw=4 fdm=syntax: */
