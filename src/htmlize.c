#define _POSIX_C_SOURCE 200809L
#include "include/htmlize.h"

#include "constants.h"
#include "include/escape.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * _POSIX_C_SOURCE  - getline, strndup
 * ctype.h			- isdigit, isspace
 * stdbool.h		- bool, true, false
 * stdio.h			- getline, FILE
 * stdlib.h			- free, strtol, exit, EXIT_SUCCESS, EXIT_FAILURE
 * string.h			- strncmp, strchr, strrchr
 */

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
	struct link   *links;
};
struct link {
	int maxindex;
	char *link;
	struct link **links;
};

/* Function prototypes */
static char *get_linkdef			(struct link *, const char *);
static void free_linkdef			(struct link *, const char *);
static void set_linkdef				(struct link *, const char *, char *);
static void free_links_recursively	(struct link *);
static void get_next_line			(struct data *);
static int _LINKDEF					(struct data *);
static int _PREFORMATTED			(struct data *);
static int _CODE					(struct data *);

static int (*LINEWISE_FUNCTIONS[])(struct data *) = {
	&_PREFORMATTED,
	&_LINKDEF,
};
static int (*CHARWISE_FUNCTIONS[])(struct data *) = {
	&_CODE,
};

static char *
get_linkdef(struct link *links, const char *id)
{
	/* Validate id */
	{
		char *end;
		if ((end = strchr(id, ']')) == NULL)
			goto invalid;

		for (const char *c = id; c < end; c++)
			if (*c != '.' && !isdigit(*c))	// Only numbers and '.' allowed
				goto invalid;
	}

	int index;
	char *end;
	index = (int)strtol(id, &end, 10);

	if (index == 0)
	{
		fputs("ID must not be zero.", stderr);
		goto invalid;
	}

	if (links->maxindex < index)
		exit((fputs("Link not defined\n", stderr), EXIT_FAILURE));

	if (*end == ']')
		return links->links[index]->link;
	else /* (*end == '.') */
		return get_linkdef(links->links[index], end + 1);

invalid:
	exit((fputs("Invalid link ID.\n", stderr), EXIT_FAILURE));
}

static void
free_linkdef(struct link *links, const char *id)
{
	// TODO. Free linkdef. And free its parent structure if it becomes empty.
}

static void
set_linkdef(struct link *links, const char *id, char *href)
{
	int index;
	char *end;
	index = (int)strtol(id, &end, 10);

	if (index == 0)
		exit((fputs("Invalid link ID. ID must not be zero.\n", stderr), EXIT_FAILURE));

	if (links->maxindex == 0)
	{
		/* Allocate */
		if ((links->links = calloc(index + 1, sizeof(struct link *))) == NULL)
			exit((perror("calloc failed"), EXIT_FAILURE));

		/* Initialize */
		links->link = NULL;
		links->maxindex = index;
		for (int i = 0; i <= index; i++)
			links->links[i] = NULL;
	}
	else if (links->maxindex < index)
	{
		/* Allocate new indices */
		if ((links->links = realloc(links->links, (index + 1) * sizeof(struct link *))) == NULL)
			exit((perror("realloc failed"), EXIT_FAILURE));

		/* Initialize new indices */
		for (int i = links->maxindex + 1; i <= index; i++)
			links->links[i] = NULL;
		links->maxindex = index;
	}

	if (*end == ']')
	{
		if (links->links[index] != NULL)
			exit((fputs("link already defined. please use it before defining again.\n", stderr), EXIT_FAILURE));

		/* Allocate */
		if ((links->links[index] = malloc(sizeof(struct link))) == NULL)
			exit((perror("malloc failed"), EXIT_FAILURE));

		/* Initialize */
		links->links[index]->maxindex = 0;
		links->links[index]->links = NULL;
		links->links[index]->link = href;
	}
	else /* (*end == '.') */
		set_linkdef(links->links[index], end + 1, href);
}

static void
free_links_recursively(struct link *links)
#define free(ptr) { free(ptr); ptr = NULL; }
{
	if (links->link != NULL)
		free(links->link);

	if (links->maxindex == 0)
		return;

	for (int index = 0; index <= links->maxindex; index++)
	{
		if (links->links[index] == NULL) continue;
		else
		{
			free_links_recursively(links->links[index]);
			free(links->links[index]);
		}
	}
}
#undef free

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
#define curline data->lines->curline
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
	 * If [1] is defined, [1.1] should not be defined, and vice-versa.
	 * A link defined once can be used only once. After that, it is undefined.
	 *
	 * So, if we define [1], and then we use [1] somewhere, and then we define
	 * [1.1], it is valid. No errors shall be shown.
	 */

	char *id;
	char *end;
	char *href;

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

	/* Validate id */
	id = curline + 2;	// \t[
	end = strchr(id, ']');
	if (end[-1] == '.')	// [1.] is not allowed, only [1] or [1.1]
		goto nope;
	for (char *c = id; c < end; c++)
		if (*c != '.' && !isdigit(*c))	// Only numbers and '.' allowed
			goto nope;

	/* Set */
	set_linkdef(data->links, id, href);
	return 1;

nope:
	free(href);
	return 0;
}
#undef curline

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

static int
_CODE(struct data *data)
#define curline data->lines->curline
{
	if (*curline != '`')
		return 0;

	fputs("<code>", data->files->out);
	for (curline++; *curline != '`'; curline++)
	{
		if (*curline == '\0') get_next_line(data);
		if (curline == NULL) break;
		fputc_escaped(*curline, data->files->out);
	}
	fputs("</code>", data->files->out);
	if (*curline == '`') curline++;

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
	struct link links;
	struct data data;

	/* Populate links */
	links.link = NULL;
	links.maxindex = 0;
	links.links = NULL;

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

		while (*lines.curline != '\0')
		{
			/* Check if somebody cares */
			for (int i = 0; i < (sizeof(CHARWISE_FUNCTIONS)/sizeof(CHARWISE_FUNCTIONS[0])); i++)
				if ((CHARWISE_FUNCTIONS[i])(&data))
					goto somebody_did_something;

			/* Nobody cares */
			fputc_escaped(*lines.curline, files.out);
			lines.curline++;

somebody_did_something:
			/* No need to lines.curline++, as it has already been done by the
			 * somebody who did something */
			continue;
		}
	}

	free_links_recursively(data.links);
	return EXIT_SUCCESS;
}

/*
 * SYNTAX -
 * 	*bold*
 * 	_italic_
 */

/* vim:set noet nowrap ts=4 sts=4 sw=4 fdm=syntax: */
