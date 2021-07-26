#include <ctype.h>
#include <stdio.h>
#include <string.h>

/*
 * ctype.h	- isalnum(), isalpha(), etc.
 * stdio.h	- printf(), fopen(), fprintf(), etc
 * string.h	- str*(), mem*()
 */

#include "include/debug.h"
#include "include/escape.h"
#include "include/htmlize.h"
#include "include/stoi.h"
#include "include/urlencode.h"
#include "constants.h"

/*
 * Named Character References that will be recognized by htmlize()
 *
 * Say a named Character Reference is "&abcd;", but hasn't been included in
 * this array. htmlize() will turn it to "&nbsp;abcd;". So, the browser shall
 * show "&abcd;" literally instead of the character that was originally
 * referenced by "&abcd;"
 */
static const char *named_references[] =
{
	"&amp;" ,
	"&nbsp;",
	"&reg;" , "&REG;" ,
	"&copy;", "&COPY;",
	"&mdash", "&ndash",
};

/*
 * Note that this function only checks from the beginning of the string. It
 * does not match the whole string.
 *
 * i.e. this function in python would be -
 *	def is_named_charref(given_str):
 *		for i in range(0,len(named_references)):
 *			if given_str.startswith(named_references[i]):
 *				return True
 *		else:
 *			return False
 */
int
is_named_charref(const char *given_str)
{
	for (int i=0; i < (sizeof(named_references)/sizeof(named_references[0])); i++)
		if (!strncmp(given_str, named_references[i], strlen(named_references[i])))
			return 1;
	return 0;
}


static void shift_lines         (struct data *);
static void get_next_line       (struct data *);
static void parse_line          (struct data *);
static int  CHECK_END           (struct data *);
static int  BLOCKQUOTE          (struct data *);
static int  HEADINGS            (struct data *);


/**** [START] Utility functions ****/
static inline void
toggle(enum bool *val)
{
	if (*val)
		*val = false;
	else
		*val = true;
}

static void
shift_lines(struct data *ptr)
{
	DPUTS("ENTER: shift_lines()\n");
	for (int i = 0; i < ptr->nlines; i++)
		memmove(ptr->lines[i], ptr->lines[i+1], MAX_LINE_LENGTH);
	DPUTS("EXIT: shift_lines()\n");
}

static void
get_next_line(struct data *ptr)
{
	DPUTS("ENTER: get_next_line()\n");
	shift_lines(ptr);
	if(fgets(ptr->lines[ptr->nlines - 1], MAX_LINE_LENGTH, ptr->files->src) == NULL)
		ptr->lines[ptr->nlines - 1][0] = '\0';	// Mark buffer as empty
	DPUTS("EXIT: get_next_line()\n");
}
/**** [END] Utility functions ****/


/**** [START] Line-wise functions ****/
static int
CHECK_END(struct data *ptr)
{
	DPUTS("INVOKE: CHECK_END()\n");
	/* Returns 0 if END, 1 otherwise */
	if (!memcmp(ptr->line, "", 1))
		return 0;
	if (!memcmp(ptr->line, "---\n", 5))
		return 0;
	if (!memcmp(ptr->line, "\\---\n", 6))
		fputs("---\n", ptr->files->dest);
	return 1;
}

static int
BLOCKQUOTE(struct data *ptr)
{
	if (!memcmp(ptr->line, "\\```", 4))
	{
		fputs("```\n", ptr->files->dest);
		return 0;
	}
	if (!memcmp(ptr->line, "```", 3))
	{
		fputs("<pre>\n", ptr->files->dest);
		get_next_line(ptr);
		while (memcmp(ptr->line, "```", 3) && memcmp(ptr->line, "", 1))
		{
			fputs_escaped(ptr->line, ptr->files->dest);
			get_next_line(ptr);
		}
		get_next_line(ptr);
		fputs("</pre>\n", ptr->files->dest);
		return 0;
	}
	return 1;
}

static int
FOOTNOTES(struct data *ptr)
{
	if (!memcmp(ptr->line, "\\^^^\n", 5))
	{
		fputs("^^^\n", ptr->files->dest);
		return 0;
	}
	if (!memcmp(ptr->line, "^^^\n", 4))
	{
		DPUTS("ENTER FOOTNOTES() MODE\n");
		fputs("<p id=\"footnotes\">\n", ptr->files->dest);
		get_next_line(ptr);
		while (CHECK_END(ptr))
		{
			get_next_line(ptr);
		}
		return 0;
	}
	return 1;
}
/**** [END] Line-wise functions ****/


/**** [START] Character-wise functions ****/
static int
HEADINGS(struct data *ptr)
{
	/* If we aren't on the first character of the line, it ain't our job */
	if (ptr->line != ptr->lines[CONTEXT_LINES])
		return 1;

	/* Huh? First character isn't a '#'? Hmm... */
	if (ptr->line[0] != '#')
	{
		/* Has somebody escaped the heading using backslashes? */
		if (ptr->line[0] == '\\' && ptr->line[1] == '#')
		{
			fputc('#', ptr->files->dest);
			ptr->line += 2;	// +2 because '\\' and '#'
			/* The above line is safe because currently,
			 * ptr->line points to the first character of
			 * the actual line, and we know that
			 * MAX_LINE_LENGTH >= 3
			 */
			return 0;	// We did our job
		}
		else return 1;	// Not our business
	}

	static int H_LEVEL;
	H_LEVEL = 0;
	DPRINTF("H_LEVEL (before): %i\n", H_LEVEL);
	while (ptr->line[0] == '#')
	{
		H_LEVEL++;
		ptr->line++;
	}
	DPRINTF("H_LEVEL (after): %i\n", H_LEVEL);
	while (ptr->line[0] == ' ')
		ptr->line++;

	/*
	 * Create an ID for the heading
	 *	- Alphabets and numbers are kept as-is
	 *	- Spaces are transformed into '-'s
	 *	- Anything else is discarded
	 */
	static char h_id[MAX_LINE_LENGTH];
	for (int i=0; i < MAX_LINE_LENGTH; i++)
		h_id[i] = '\0';
	for (int i=0,j=0; i < MAX_LINE_LENGTH && ptr->line[i] != '\0'; i++)
		if (isalnum(ptr->line[i]))
			h_id[j++] = ptr->line[i];
		else if (ptr->line[i] == ' ')
			h_id[j++] = '-';

	/* Print the opening HTML tags */
	fprintf(ptr->files->dest,
			"<h%i id=\"%s\"><a class=\"self-link\" href=\"#%s\">",
			H_LEVEL, h_id, h_id);

	/* Parse the remaining of the line */
	parse_line(ptr);

	/* Print the closing HTML tags */
	fprintf(ptr->files->dest, "</a></h%u>\n", H_LEVEL);
	return 0;
}

static int
CHARREFS(struct data *ptr)
{
	/* For it to be a charref, the current character MUST be ampersand */
	if (ptr->line[0] != '&')
		return 1;
	/* If the ampersand isn't the very first character in the current line,
	 * then check whether the previous character was a backslash. If it
	 * was, then it means that this ampersand was escaped. */
	if ((ptr->line != ptr->lines[CONTEXT_LINES]) && ptr->line[-1] == '\\')
		return 1;
}

static void
parse_line(struct data *ptr)
{
	while (ptr->line[0] != '\n' && ptr->line[0] != '\0')
	{
		/* Check if the current character is worth anything to anyone */
		if (!HEADINGS(ptr) || !CHARREFS(ptr) || !1) // XXX: Replace the 1 with any new function
			continue;

		/* Nobody cared. fputc_escaped() it and move on. */
		fputc_escaped(ptr->line[0], ptr->files->dest);
		ptr->line++;
	}
}
/**** [END] Character-wise functions ****/


int
htmlize(FILE *src, FILE *dest)
{
	if (MAX_LINE_LENGTH < 5)
	{
		/*
		 * To mark the end of current input, we need to read the string
		 * "---\n" into the buffer. Since "---\n" is actually the array
		 * {'-', '-', '-', '\n', '\0'}, which contains 5 elements, we
		 * need to have MAX_LINE_LENGTH >= 5
		 *
		 * Since we don't, BAIL OUT!
		 */
		fputs("htmlize(): MAX_LINE_LENGTH too low", stderr);
		return -1;
	}

	struct data data;
	struct data *ptr;
	ptr = &data;

	struct files files;
	data.files	= &files;
	files.src	= src;
	files.dest	= dest;

	struct config config;
	data.config		= &config;
	config.BOLD_OPEN	= false;
	config.ITALIC_OPEN	= false;
	config.CODE_OPEN	= false;
	config.HTML_TAG_OPEN	= false;
	config.LINK_OPEN	= false;
	config.LINK_TEXT_OPEN	= false;
	config.TABLE_MODE	= false;
	config.LIST_MODE	= false;

	/* Calculate data.lines size and check if it is correct */
	if ((CONTEXT_LINES*2 + 1) == (sizeof(data.lines)/sizeof(data.lines[0])))
		data.nlines = sizeof(data.lines)/sizeof(data.lines[0]);
	else
	{
		fputs("FATAL: htmlize(): (CONTEXT_LINES*2 + 1) != (sizeof(data.lines)/sizeof(data.lines[0]))\n", stderr);
		fprintf(stderr, "CONTEXT_LINES*2 + 1 = %i\n", CONTEXT_LINES*2 + 1);
		fprintf(stderr, "sizeof(data.lines)/sizeof(data.lines[0]) = %i\n", (int)(sizeof(data.lines)/sizeof(data.lines[0])));
		return -10;
	}

	/* Initialize data.lines with empty lines */
	for (int i = 0; i < data.nlines; i++)
		for (int j = 0; j < MAX_LINE_LENGTH; j++)
			data.lines[i][j] = '\0';

	/* Populate data.lines, but stop immediately at EOF */
	for (int i = CONTEXT_LINES; i < data.nlines; i++)
		if (fgets(data.lines[i], MAX_LINE_LENGTH, src) == NULL)
			break;

	/* If the first line we read is empty, that means we hit EOF as-soon-as
	 * we started reading. ie. We received no input. */
	if (!memcmp(data.lines[CONTEXT_LINES], "", 1))
		return 1;	// No input

	ptr->line = ptr->lines[CONTEXT_LINES];
	while (CHECK_END(ptr))
	{
		BLOCKQUOTE(ptr);
		FOOTNOTES(ptr);

		parse_line(ptr);

		get_next_line(ptr);
	}
	return 0;
}

// void
// htmlize(FILE *in, FILE *out)
// /*
//  * Things that are escaped using '\\' -
//  *	- \```\n
//  *	- \`code\`
//  *	- \*bold\*
//  *	- \_italic\_
//  *	- \<HTML>
//  *	- Table \| cells
//  *	- \&nbsp; HTML Named char refs
//  *	- \&#...; HTML Numeric char ref
//  *	- \!(ID)[Link text\]
//  *	- Footnotes\[^10]
//  */
// /*
//  * Things implemented -
//  *	- ```
//  *	- `code`
//  *	- *bold*
//  *	- _italic_
//  *	- <table>
//  *	- # Headings
//  *	- Lists
//  *	- HTML <tags>
//  *	- Linebreak if two spaces at line end
//  *	- &nbsp;  named character references (names defined in constants.h)
//  *	- &#...;  numeric character references
//  *	- <br> at Blank lines with two spaces
//  *	- Links
//  *	- Footnotes[^10]
//  */
// {
// 
// #define HISTORY_SIZE 2
// 	char lines[HISTORY_SIZE*2 + 1][MAX_LINE_LENGTH];
// 	char *original_line = lines[HISTORY_SIZE + 1];
// 	char *last_line = lines[HISTORY_SIZE];
// 	char line[MAX_LINE_LENGTH];				// The buffer we shall work on
// 
// 	char links[MAX_LINKS][MAX_LINE_LENGTH];	// +1 because indexing starts at 0
// 
// 	enum bool {false, true};
// 	enum bool BOLD_OPEN         = false;
// 	enum bool ITALIC_OPEN       = false;
// 	enum bool CODE_OPEN         = false;
// 	enum bool CODEBLOCK_OPEN    = false;
// 	enum bool HTML_TAG_OPEN     = false;
// 	enum bool LINK_OPEN         = false;
// 	enum bool LINK_TEXT_OPEN    = false;
// 	enum bool TABLE_MODE        = false;
// 	enum bool LIST_MODE         = false;
// 	enum bool FOOTNOTE_MODE     = false;
// 
// 	unsigned int H_LEVEL = 0;
// 
// 	char *pch;	// (p)revious (ch)aracter
// 	char *cch;	// (c)urrent  (ch)aracter
// 	char *nch;	// (n)ext     (ch)aracter
// 
// 	for (;;)
// 	{
// 		// Save current line (unmodified) in last_line
// 		memmove(last_line, original_line, MAX_LINE_LENGTH);
// 
// 		/*
// 		 * Read and store a line from *in into line[]
// 		 * Break loop if we've reached EOF (i.e. fgets() == NULL)
// 		 */
// 		if (fgets(line, MAX_LINE_LENGTH, in) == NULL)
// 			break;
// 
// 		// Save the unmodified line into a buffer for future reference
// 		memmove(original_line, line, MAX_LINE_LENGTH);
// 
// 		/*
// 		 * NOTE: Always remember, CODEBLOCKs take precedence over anything else
// 		 */
// 
// 		// For <pre> blocks
// 		if (!memcmp(line, "\\```", 3))	// Escaped by a backslash
// 		{
// 			fputs("```\n", out);
// 			continue;
// 		}
// 		if (!memcmp(line, "```", 3))
// 		{
// 			++CODEBLOCK_OPEN;
// 			if (CODEBLOCK_OPEN %= 2)
// 				fputs("<pre>\n", out);
// 			else
// 				fputs("</pre>\n", out);
// 			continue;
// 		}
// 		if (CODEBLOCK_OPEN)
// 		{
// 			// Blindly escape everything and move on
// 			fputs_escaped(line, out);
// 			continue;
// 		}
// 
// 
// 		// End of blog
// 		if (!memcmp(line, "---\n", 4))
// 			break;
// 
// 
// 		// Footnotes
// 		if (!memcmp(line, "^^^\n", 4))
// 		{
// 			FOOTNOTE_MODE = 1;
// 			fputs("<p id=\"footnotes\">\n", out);
// 			continue;
// 		}
// 
// 
// 		// Link definition
// 		if (!memcmp(line, "\t[", 2))
// 		{
// 			int id;
// 			char *p;
// 			char *href;
// 			char *start;
// 			char *end;
// 
// 			start = line + 2;	// +2 for the '\t['
// 			*(end = memchr(line, ']', MAX_LINE_LENGTH-1)) = '\0'; // mark the end
// 			id = stoi(start);
// 
// 			if (id > MAX_LINKS)
// 			{
// 				fprintf(stderr,
// 						"link index %i is bigger than MAX_LINKS (%i)",
// 						id, MAX_LINKS);
// 				continue;
// 			}
// 
// 			href = end + 2;							// +2 for the ']:'
// 			while (*href == ' ' || *href == '\t')	// Skip whitespaces, if any
// 				href++;
// 
// 			/* Turn the trailing newline into string-terminator */
// 			*(p = memchr(href, '\n', MAX_LINE_LENGTH - (href-line))) = '\0';
// 
// 			memmove(links[id], href, MAX_LINE_LENGTH - (href-line));
// 			continue;
// 		}
// 
// 
// 		// Blank line
// 		if (!memcmp(line, "  \n", 3) || !memcmp(line, "\n", 2))
// 		{
// 			if (!memcmp(last_line, "\t[", 2))
// 				/*
// 				 * Last line was a link definition.
// 				 * So, this empty line was just inserted for
// 				 * readablity.  Ignore it.
// 				 */
// 				continue;
// 
// 			if (!memcmp(last_line, "\n", 2))
// 				continue;
// 
// 			fputs("</p>\n<p>\n", out);
// 			continue;
// 		}
// 
// 
// 		// Lists
// 		if (!memcmp(line, "<ul", 3) || !memcmp(line, "<ol", 3))
// 		{
// 			LIST_MODE = 1;
// 			fputs(line, out);
// 			continue;
// 		}
// 		if (!memcmp(line, "</ul", 4) || !memcmp(line, "</ol", 4))
// 		{
// 			LIST_MODE = 0;
// 			fputs(line, out);
// 			continue;
// 		}
// 		if (LIST_MODE)
// 		{
// 			char *p;
// 			if ((p = memchr(line, '-', MAX_LINE_LENGTH)) != NULL)
// 			{
// 				/*
// 				 * Check if the found '-' is the first char of the line
// 				 *
// 				 * ie. check whether there exists any non-blankspace character
// 				 * on this line before the '-' character we found
// 				 */
// 				char *i = p;
// 				while (i > line)
// 					if (!isblank(*--i))		// We found a non-blank character
// 						break;				// break out
// 
// 				if (i == line)
// 					/*
// 					 * ie. we didn't "break" out of the previous loop
// 					 * That means our '-' *is* the first character of the line
// 					 * So, it should be turned into <li>
// 					 */
// 				{
// 
// 					/* Move the text forward by 3 spaces to make space for the <li> tag */
// 					memmove(p+3, p, MAX_LINE_LENGTH-(p-line)-3);
// 
// 					/* Add the <li> tag */
// 					memmove(p, "<li>", 4);	// 4, NOT 5, bcoz we DO NOT want the last \0
// 
// 					/* Remove extra whitespaces, if any */
// 					p += 4;		// p now points to the char just after the <li> tag
// 					while (*p == ' ')
// 						memmove(p, p + 1, MAX_LINE_LENGTH-(p-line));
// 				}
// 			}
// 		}
// 
// 
// 		// Tables
// 		if (!memcmp(line, "<table", 6))
// 		{
// 			TABLE_MODE = 1;
// 			fputs(line, out);
// 			continue;
// 		}
// 		if (TABLE_MODE && !memcmp(line, "</table", 7))
// 		{
// 			TABLE_MODE = 0;
// 			fputs(line, out);
// 			continue;
// 		}
// 
// 
// 		// Headings (NOTE: starts from h2)
// 		if (line[0] == '#')
// 		{
// 			// Calculate H_LEVEL
// 			H_LEVEL = 1;
// 			while (line[H_LEVEL++] == '#') {;}
// 
// 			// Remove leading #'s and spaces
// 			memmove(&line[0], &line[H_LEVEL - 1], MAX_LINE_LENGTH - (H_LEVEL - 1));	// #'s
// 			while (line[0] == ' ')													// spaces
// 				memmove(line, line + 1, MAX_LINE_LENGTH - 1);
// 
// 			/*
// 			 * Create an ID for the heading
// 			 *	- Alphabets and numbers are kept as-is
// 			 *	- Spaces are transformed into '-'s
// 			 *	- Anything else is discarded
// 			 */
// 			char h_id[MAX_LINE_LENGTH] = "";
// 			for (int i=0,j=0; i < MAX_LINE_LENGTH; i++)
// 				if (line[i] == '\0')
// 					/*
// 					 * NOTE: because of the way we initialized h_id,
// 					 * all of the characters in the array are initialized to '\0'
// 					 * So, we don't need to add the \0 terminator here
// 					 */
// 					break;
// 				else if (isalnum(line[i]))
// 					h_id[j++] = line[i];
// 				else if (line[i] == ' ')
// 					h_id[j++] = '-';
// 
// 			fprintf(
// 					out,
// 					"<h%i id=\"%s\"><a class=\"self-link\" href=\"#%s\">",
// 					H_LEVEL, h_id, h_id
// 				   );
// 		}
// 
// 
// 		// Footnotes
// 		if (FOOTNOTE_MODE)
// 		{
// 			char *id_end;
// 			char  id[MAX_LINE_LENGTH];
// 
// 			id_end = memchr(line, ':', MAX_LINE_LENGTH);
// 			if (id_end == NULL)
// 				continue;
// 
// 			*id_end = '\0';
// 			memmove(id, line, MAX_LINE_LENGTH);
// 			*id_end = ':';
// 
// 			fprintf(out, "<a class=\"footnote\" id=\"fn:%s\" href=\"#fnref:%s\">[%s]</a>",
// 					id, id, id);
// 
// 			memmove(line, id_end, MAX_LINE_LENGTH - (id_end-line));	// Skip till ':'
// 
// 			/* no continue */
// 		}
// 
// 
// 
// 		// Line prefix for tables
// 		if (TABLE_MODE)
// 			fputs("<tr><td>", out);
// 
// 
// 
// 		/* Initialize variables to avoid undefined behaviour */
// 		pch = line;
// 		cch = line;
// 		nch = line;
// 		/*
// 		 * Yes, I know the values are nonsense
// 		 * But at least it's _known_ nonsense
// 		 *
// 		 * If we access it while it's undefined, it will
// 		 *   - either SEGFAULT (in which case debugging it is easy)
// 		 *   - or *silently* input _unknown_ nonsense into outfile!
// 		 */
// 
// 		/* iterate over the stored line[] */
// 		for (int index = 1; index < MAX_LINE_LENGTH; index++)
// 		{
// 			pch = cch;
// 			cch = nch;
// 			nch = line + index;
// 
// 			if (*cch == '\0')	// we've reached end of string
// 				break;			// stop iterating
// 
// 
// 			if (*cch == '\\')
// 			{
// 				if (
// 						(*nch == '`' || *nch == '*' || *nch == '_')
// 						|| (!HTML_TAG_OPEN && *nch == '<')
// 						|| (TABLE_MODE && *nch == '|')
// 						|| (!LINK_OPEN && *nch == '!' && *(nch + 1) == '(')
// 						|| (*nch == '&' && (*(nch + 1) == '#' || is_named_charref(nch)))
// 						|| (*nch == '[' && *(nch + 1) == '^')
// 				   ) {;} // print nothing
// 				else
// 					fputc('\\', out);
// 				continue;
// 			}
// 
// 
// 			// Heading tag close
// 			if (H_LEVEL && *cch == '\n')
// 			{
// 				fprintf(out, "</a></h%u>\n", H_LEVEL);
// 				H_LEVEL = 0;
// 				continue;
// 			}
// 
// 
// 			/*
// 			 * NOTE: CODE shall always come first
// 			 */
// 
// 			// `code`
// 			if (!CODE_OPEN && *cch == '`' && !(isalnum(*pch) && isalnum(*nch)) && *pch != '\\')
// 			{
// 				CODE_OPEN = 1;
// 				fputs("<code>", out);
// 				continue;
// 			}
// 			if (CODE_OPEN && *cch == '`' && *pch != '\\')
// 			{
// 				CODE_OPEN = 0;
// 				fputs("</code>", out);
// 				continue;
// 			}
// 			if (CODE_OPEN)
// 			{
// 				// Nothing to see here, just escape and move along
// 				fputc_escaped(*cch, out);
// 				continue;
// 			}
// 
// 
// 			// Linebreak if two spaces at line end
// 			if (*cch == '\n' && *pch == ' ' && *(pch-1) == ' ')
// 			{
// 				fputs("<br>\n", out);
// 				continue;
// 			}
// 
// 
// 			// HTML <tags>
// 			if ((*cch == '<') && (*nch == '/' || isalpha(*nch)) && *pch != '\\')
// 			{
// 				HTML_TAG_OPEN = 1;
// 				fputc('<', out);
// 				continue;
// 			}
// 			if (HTML_TAG_OPEN && *cch == '>')
// 			{
// 				HTML_TAG_OPEN = 0;
// 				fputc('>', out);
// 				continue;
// 			}
// 			if (HTML_TAG_OPEN)
// 			{
// 				// Nothing needs escaping, fputc() and move on
// 				fputc(*cch, out);
// 				continue;
// 			}
// 
// 
// 			// HTML Character references
// 			if (*cch == '&' && (*nch == '#' || is_named_charref(cch)))
// 			{
// 				if (*pch == '\\')		// ie. it has been escaped
// 					fputs("&amp;", out);
// 				else
// 					fputc('&', out);
// 				continue;
// 			}
// 
// 
// 			// *bold*
// 			if (*cch == '*' && !(isalnum(*pch) && isalnum(*nch)) && *pch != '\\')
// 			{
// 				++BOLD_OPEN;
// 				if (BOLD_OPEN %= 2)
// 					fputs("<strong>", out);
// 				else
// 					fputs("</strong>", out);
// 				continue;
// 			}
// 
// 
// 			// _italic_
// 			if (*cch == '_' && !(isalnum(*pch) && isalnum(*nch)) && *pch != '\\')
// 			{
// 				++ITALIC_OPEN;
// 				if (ITALIC_OPEN %= 2)
// 					fputs("<em>", out);
// 				else
// 					fputs("</em>", out);
// 				continue;
// 			}
// 
// 
// 			// Links	!(ID) attribute="value" [text]
// 			if (*cch == '!' && *nch == '(' && *pch != '\\' && !LINK_OPEN)
// 			{
// 				LINK_OPEN = 1;
// 
// 				/* Mark string end for stoi() computation */
// 				char *p = memchr(nch, ')', MAX_LINE_LENGTH - index);
// 				*p = '\0';
// 
// 				int link_id = stoi(nch + 1);	// +1 for '('
// 				if (link_id > MAX_LINKS)
// 				{
// 					fprintf(stderr,
// 							"link index %i is bigger than MAX_LINKS (%i)",
// 							link_id, MAX_LINKS);
// 					continue;
// 				}
// 				else
// 					fprintf(out, "<a href=\"%s\" ", links[link_id]);
// 
// 				/* Skip the characters we have interpreted just now and continue looping */
// 				nch = p + 1;			// nch now point at char after the ')' in "!(id)"
// 				index = nch - line;		// since (nch = line + index)
// 				continue;
// 			}
// 			if (LINK_OPEN)
// 			{
// 				if (*cch == '[')
// 				{
// 					/*
// 					 * !(ID)     [Text]
// 					 * <a href=ID>Text</a>
// 					 *           ↑
// 					 *           |
// 					 */
// 					fputc('>', out);
// 					LINK_OPEN = 0;
// 					LINK_TEXT_OPEN = 1;
// 				}
// 				else
// 				{
// 					/*
// 					 * LINK_OPEN means we are inside the < and >
// 					 * So, we mustn't escape anything
// 					 */
// 					fputc(*cch, out);
// 				}
// 				continue;
// 			}
// 			if (LINK_TEXT_OPEN && *cch == ']' && *nch != '\\')
// 			{
// 				/*
// 				 * !(ID)     [Text]
// 				 * <a href=ID>Text</a>
// 				 *                ↑
// 				 *                |
// 				 */
// 				LINK_TEXT_OPEN = 0;
// 				fputs("</a>", out);
// 				continue;
// 			}
// 
// 
// 			// Footnotes	[^1]
// 			if (*cch == '[' && *nch == '^' && *pch != '\\')
// 			{
// 				char *p;
// 				char id[MAX_LINE_LENGTH];
// 
// 				*(p = memchr(cch, ']', MAX_LINE_LENGTH - (cch-line))) = '\0';
// 				memmove(id, cch+2, MAX_LINE_LENGTH - (cch-line)); // +2 for '[^'
// 				*p = ']';
// 
// 				fprintf(out,
// 					"<a class=\"footnote\" id=\"fnref:%s\" href=\"#fn:%s\"><sup>[%s]</sup></a>",
// 					id, id, id
// 					);
// 
// 				/* skip till the character just after the ']' */
// 				nch = p + 1;
// 				cch = p;
// 				index = p - line;
// 
// 				continue;
// 			}
// 
// 			// Table cells
// 			if (TABLE_MODE)
// 			{
// 				if (*cch == '|' && *pch != '\\')
// 				{
// 					/* Cell separator */
// 					fputs("</td><td>", out);
// 					continue;
// 				}
// 				if (*cch == '\n')
// 				{
// 					/* Table row ends here */
// 					fputs("</td></tr>\n", out);
// 					continue;
// 				}
// 			}
// 
// 
// 			// For footnotes
// 			if (FOOTNOTE_MODE && *cch == '\n')
// 			{
// 				fputs("<br>\n", out);
// 				break;
// 			}
// 
// 			// It's just a simple, innocent character
// 			fputc_escaped(*cch, out);
// 		}
// 	}
// }

// vim:fdm=syntax:sw=8:sts=8:ts=8:nowrap:
