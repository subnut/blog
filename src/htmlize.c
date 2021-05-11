#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "include/htmlize.h"
#include "include/stoi.h"
#include "constants.h"

/*
 * ctype.h	- isalnum(), isalpha(), etc.
 * stdio.h	- printf(), fopen(), fprintf(), etc
 * string.h	- str*(), mem*()
 */

static const char *named_references[2] =
{
    "&nbsp;",
    "&copy;",
};

void
fputc_escaped(char c, FILE *stream)
{
	switch (c)
	{
		case '<': fputs("&lt;",  stream); break;
		case '>': fputs("&gt;",  stream); break;
		case '&': fputs("&amp;", stream); break;
		default:  fputc(c,		 stream); break;
	}
}

void
fputs_escaped(const char *s, FILE *stream)
{
	for (int i=0; s[i] != '\0'; i++)
		fputc_escaped(s[i], stream);
}

int
is_named_charref(const char *given_str)
{
	for (int i=0; i <= (sizeof(named_references)/sizeof(named_references[0])); i++)
	{
		const char *named_ref;
		int index;
		int len;

		named_ref = named_references[i];
		index = strchr(named_ref, ';') - named_ref;		// Index of ending ';'
		len = index + 1;								// Since indexing starts from 0

		if (!strncmp(given_str, named_references[i], len))
			return 1;
	}
	return 0;
}

void
htmlize(FILE *in, FILE *out)
/*
 * Things that are escaped using '\\' -
 *	- \```\n
 *	- \`code\`
 *	- \*bold\*
 *	- \_italic\_
 *	- \<HTML>
 *	- Table \| cells
 *	- \&nbsp; HTML Named char refs
 *	- \&#...; HTML Numeric char ref
 *	- \!(ID)[Link text\]
 *	- Footnotes\[^10]
 */
/*
 * Things implemented -
 *	- ```
 *	- `code`
 *	- *bold*
 *	- _italic_
 *	- <table>
 *	- # Headings
 *	- Lists
 *	- HTML <tags>
 *	- Linebreak if two spaces at line end
 *	- &nbsp;  named character references (names defined in constants.h)
 *	- &#...;  numeric character references
 *	- <br> at Blank lines with two spaces
 *	- Links
 *	- Footnotes[^10]
 */
{
	char line[MAX_LINE_LENGTH];	// The buffer we shall work on
	char original_line[MAX_LINE_LENGTH];	// The buffer used to store the current line, unmodified

	char last_line[MAX_LINE_LENGTH];
	char links[MAX_LINKS + 1][MAX_LINE_LENGTH];	// +1 because indexing starts at 0

	unsigned char BOLD_OPEN = 0;
	unsigned char ITALIC_OPEN = 0;
	unsigned char CODE_OPEN = 0;
	unsigned char CODEBLOCK_OPEN = 0;
	unsigned char HTML_TAG_OPEN = 0;
	unsigned char LINK_OPEN = 0;
	unsigned char LINK_TEXT_OPEN = 0;
	unsigned char TABLE_MODE = 0;
	unsigned char LIST_MODE = 0;
	unsigned char FOOTNOTE_MODE = 0;

	unsigned int H_LEVEL = 0;

	char *pch;	// (p)revious (ch)aracter
	char *cch;	// (c)urrent  (ch)aracter
	char *nch;	// (n)ext     (ch)aracter

	for (;;)
	{
		// Save current line (unmodified) in last_line
		memmove(last_line, original_line, MAX_LINE_LENGTH);

		/*
		 * Read and store a line from *in into line[]
		 * Break loop if we've reached EOF (i.e. fgets() == NULL)
		 */
		if (fgets(line, MAX_LINE_LENGTH, in) == NULL)
			break;

		// Save the unmodified line into a buffer for future reference
		memmove(original_line, line, MAX_LINE_LENGTH);

		/*
		 * NOTE: Always remember, CODEBLOCKs take precedence over anything else
		 */

		// For <pre> blocks
		if (!memcmp(line, "\\```", 3))	// Escaped by a backslash
		{
			fputs("```\n", out);
			continue;
		}
		if (!memcmp(line, "```", 3))
		{
			++CODEBLOCK_OPEN;
			if (CODEBLOCK_OPEN %= 2)
				fputs("<pre>\n", out);
			else
				fputs("</pre>\n", out);
			continue;
		}
		if (CODEBLOCK_OPEN)
		{
			// Blindly escape everything and move on
			fputs_escaped(line, out);
			continue;
		}


		// End of blog
		if (!memcmp(line, "---\n", 4))
			break;


		// Footnotes
		if (!memcmp(line, "===\n", 4))
		{
			FOOTNOTE_MODE = 1;
			fputs("<p id=\"footnotes\">\n", out);
			continue;
		}


		// Link definition
		if (!memcmp(line, "\t[", 2))
		{
			int id;
			char *p;
			char *href;
			char *start;
			char *end;

			start = line + 2;	// +2 for the '\t['
			*(end = memchr(line, ']', MAX_LINE_LENGTH-1)) = '\0'; // mark the end
			id = stoi(start);

			if (id > MAX_LINKS)
			{
				fprintf(stderr,
						"link index %i is bigger than MAX_LINKS (%i)",
						id, MAX_LINKS);
				continue;
			}

			href = end + 2;							// +2 for the ']:'
			while (*href == ' ' || *href == '\t')	// Skip whitespaces, if any
				href++;

			/* Turn the trailing newline into string-terminator */
			*(p = memchr(href, '\n', MAX_LINE_LENGTH - (href-line))) = '\0';

			memmove(links[id], href, MAX_LINE_LENGTH - (href-line));
			continue;
		}


		// Blank line with two spaces
		if (!memcmp(line, "  \n", 3) || !memcmp(line, "\n", 2))
		{
			fputs("<br>\n", out);
			continue;
		}


		// Lists
		if (!memcmp(line, "<ul", 3) || !memcmp(line, "<ol", 3))
		{
			LIST_MODE = 1;
			fputs(line, out);
			continue;
		}
		if (!memcmp(line, "</ul", 4) || !memcmp(line, "</ol", 4))
		{
			LIST_MODE = 0;
			fputs(line, out);
			continue;
		}
		if (LIST_MODE)
		{
			char *p;
			if ((p = memchr(line, '-', MAX_LINE_LENGTH)) != NULL)
			{
				/*
				 * Check if the found '-' is the first char of the line
				 *
				 * ie. check whether there exists any non-blankspace character
				 * on this line before the '-' character we found
				 */
				char *i = p;
				while (i > line)
					if (!isblank(*--i))		// We found a non-blank character
						break;				// break out

				if (i == line)
					/*
					 * ie. we didn't "break" out of the previous loop
					 * That means our '-' *is* the first character of the line
					 * So, it should be turned into <li>
					 */
				{

					/* Move the text forward by 3 spaces to make space for the <li> tag */
					memmove(p+3, p, MAX_LINE_LENGTH-(p-line)-3);

					/* Add the <li> tag */
					memmove(p, "<li>", 4);	// 4, NOT 5, bcoz we DO NOT want the last \0

					/* Remove extra whitespaces, if any */
					p += 4;		// p now points to the char just after the <li> tag
					while (*p == ' ')
						memmove(p, p + 1, MAX_LINE_LENGTH-(p-line));
				}
			}
		}


		// Tables
		if (!memcmp(line, "<table", 6))
		{
			TABLE_MODE = 1;
			fputs(line, out);
			continue;
		}
		if (TABLE_MODE && !memcmp(line, "</table", 7))
		{
			TABLE_MODE = 0;
			fputs(line, out);
			continue;
		}


		// Headings (NOTE: starts from h2)
		if (line[0] == '#')
		{
			// Calculate H_LEVEL
			H_LEVEL = 1;
			while (line[H_LEVEL++] == '#') {;}

			// Remove leading #'s and spaces
			memmove(&line[0], &line[H_LEVEL - 1], MAX_LINE_LENGTH - (H_LEVEL - 1));	// #'s
			while (line[0] == ' ')									 				// spaces
				memmove(line, line + 1, MAX_LINE_LENGTH - 1);

			/*
			 * Create an ID for the heading
			 *	- Alphabets and numbers are kept as-is
			 *	- Spaces are transformed into '-'s
			 *	- Anything else is discarded
			 */
			char h_id[MAX_LINE_LENGTH] = "";
			for (int i=0,j=0; i < MAX_LINE_LENGTH; i++)
				if (line[i] == '\0')
					/*
					 * NOTE: because of the way we initialized h_id,
					 * all of the characters in the array are initialized to '\0'
					 * So, we don't need to add the \0 terminator here
					 */
				   	break;
				else if (isalnum(line[i]))
					h_id[j++] = line[i];
				else if (line[i] == ' ')
					h_id[j++] = '-';

			fprintf(
					out,
					"<h%i id=\"%s\"><a class=\"self-link\" href=\"#%s\">",
					H_LEVEL, h_id, h_id
				   );
		}


		// Footnotes
		if (FOOTNOTE_MODE)
		{
			char *id_end;
			char  id[MAX_LINE_LENGTH];

			id_end = memchr(line, ':', MAX_LINE_LENGTH);
			if (id_end == NULL)
				continue;

			*id_end = '\0';
			memmove(id, line, MAX_LINE_LENGTH);
			*id_end = ':';

			fprintf(out, "<a class=\"footnote\" id=\"fn:%s\" href=\"#fnref:%s\">[%s]</a>",
					id, id, id);

			memmove(line, id_end, MAX_LINE_LENGTH - (id_end-line));	// Skip till ':'

			/* no continue */
		}



		// Line prefix for tables
		if (TABLE_MODE)
			fputs("<tr><td>", out);



		/* Initialize variables to avoid undefined behaviour */
		pch = line;
		cch = line;
		nch = line;
		/*
		 * Yes, I know the values are nonsense
		 * But at least it's _known_ nonsense
		 *
		 * If we access it while it's undefined, it will
		 *   - either SEGFAULT (in which case debugging it is easy)
		 *   - or *silently* input _unknown_ nonsense into outfile!
		 */

		/* iterate over the stored line[] */
		for (int index = 1; index < MAX_LINE_LENGTH; index++)
		{
			pch = cch;
			cch = nch;
			nch = line + index;

			if (*cch == '\0')	// we've reached end of string
				break;			// stop iterating


			if (*cch == '\\')
			{
				if (
						(*nch == '`' || *nch == '*' || *nch == '_')
						|| (!HTML_TAG_OPEN && *nch == '<')
						|| (TABLE_MODE && *nch == '|')
						|| (!LINK_OPEN && *nch == '!' && *(nch + 1) == '(')
						|| (*nch == '&' && (*(nch + 1) == '#' || is_named_charref(nch)))
						|| (*nch == '[' && *(nch + 1) == '^')
				   ) {;}
				else
					fputc('\\', out);
				continue;
			}


			// Heading tag close
			if (H_LEVEL && *cch == '\n')
			{
				fprintf(out, "</a></h%u>\n", H_LEVEL);
				H_LEVEL = 0;
				continue;
			}


			/*
			 * NOTE: CODE shall always come first
			 */

			// `code`
			if (!CODE_OPEN && *cch == '`' && !(isalnum(*pch) && isalnum(*nch)) && *pch != '\\')
			{
				CODE_OPEN = 1;
				fputs("<code>", out);
				continue;
			}
			if (CODE_OPEN && *cch == '`' && *pch != '\\')
			{
				CODE_OPEN = 0;
				fputs("</code>", out);
				continue;
			}
			if (CODE_OPEN)
			{
				// Nothing to see here, just escape and move along
				fputc_escaped(*cch, out);
				continue;
			}


			// Linebreak if two spaces at line end
			if (*cch == '\n' && *pch == ' ' && *(pch-1) == ' ')
			{
				fputs("<br>\n", out);
				continue;
			}


			// HTML <tags>
			if ((*cch == '<') && (*nch == '/' || isalpha(*nch)) && *pch != '\\')
			{
				HTML_TAG_OPEN = 1;
				fputc('<', out);
				continue;
			}
			if (HTML_TAG_OPEN && *cch == '>')
			{
				HTML_TAG_OPEN = 0;
				fputc('>', out);
				continue;
			}
			if (HTML_TAG_OPEN)
			{
				// Nothing needs escaping, fputc() and move on
				fputc(*cch, out);
				continue;
			}


			// HTML Character references
			if (*cch == '&' && (*nch == '#' || is_named_charref(cch)))
			{
				if (*pch == '\\')		// ie. it has been escaped
					fputs("&amp;", out);
				else
					fputc('&', out);
				continue;
			}


			// *bold*
			if (*cch == '*' && !(isalnum(*pch) && isalnum(*nch)) && *pch != '\\')
			{
				++BOLD_OPEN;
				if (BOLD_OPEN %= 2)
					fputs("<strong>", out);
				else
					fputs("</strong>", out);
				continue;
			}


			// _italic_
			if (*cch == '_' && !(isalnum(*pch) && isalnum(*nch)) && *pch != '\\')
			{
				++ITALIC_OPEN;
				if (ITALIC_OPEN %= 2)
					fputs("<em>", out);
				else
					fputs("</em>", out);
				continue;
			}


			// Links	!(ID) attribute="value" [text]
			if (*cch == '!' && *nch == '(' && *pch != '\\' && !LINK_OPEN)
			{
				LINK_OPEN = 1;

				/* Mark string end for stoi() computation */
				char *p = memchr(nch, ')', MAX_LINE_LENGTH - index);
				*p = '\0';

				int link_id = stoi(nch + 1);	// +1 for '('
				if (link_id > MAX_LINKS)
				{
					fprintf(stderr,
							"link index %i is bigger than MAX_LINKS (%i)",
							link_id, MAX_LINKS);
					continue;
				}
				else
					fprintf(out, "<a href=\"%s\" ", links[link_id]);

				/* Skip the characters we have interpreted just now and continue looping */
				nch = p + 1;			// nch now point at char after the ')' in "!(id)"
				index = nch - line;		// since (nch = line + index)
				continue;
			}
			if (LINK_OPEN)
			{
				if (*cch == '[')
				{
					/*
					 * !(ID)     [Text]
					 * <a href=ID>Text</a>
					 *           ↑
					 *           |
					 */
					fputc('>', out);
					LINK_OPEN = 0;
					LINK_TEXT_OPEN = 1;
				}
				else
				{
					/*
					 * LINK_OPEN means we are inside the < and >
					 * So, we mustn't escape anything
					 */
					fputc(*cch, out);
				}
				continue;
			}
			if (LINK_TEXT_OPEN && *cch == ']' && *nch != '\\')
			{
				/*
				 * !(ID)     [Text]
				 * <a href=ID>Text</a>
				 *                ↑
				 *                |
				 */
				LINK_TEXT_OPEN = 0;
				fputs("</a>", out);
				continue;
			}


			// Footnotes	[^1]
			if (*cch == '[' && *nch == '^' && *pch != '\\')
			{
				char *p;
				char id[MAX_LINE_LENGTH];

				*(p = memchr(cch, ']', MAX_LINE_LENGTH - (cch-line))) = '\0';
				memmove(id, cch+2, MAX_LINE_LENGTH - (cch-line)); // +2 for '[^'
				*p = ']';

				fprintf(out,
					"<a class=\"footnote\" id=\"fnref:%s\" href=\"#fn:%s\"><sup>[%s]</sup></a>",
					id, id, id
					);

				/* skip till the character just after the ']' */
				nch = p + 1;
				cch = p;
				index = p - line;

				continue;
			}

			// Table cells
			if (TABLE_MODE)
			{
				if (*cch == '|' && *pch != '\\')
				{
					/* Cell separator */
					fputs("</td><td>", out);
					continue;
				}
				if (*cch == '\n')
				{
					/* Table row ends here */
					fputs("</td></tr>\n", out);
					continue;
				}
			}


			// For footnotes
			if (FOOTNOTE_MODE && *cch == '\n')
			{
				fputs("<br>\n", out);
				break;
			}

			// It's just a simple, innocent character
			fputc_escaped(*cch, out);
		}
	}
}

// vim:fdm=syntax:
