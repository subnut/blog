#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define MAX_LINE_LENGTH 500

void fputc_escaped(char c, FILE *stream)
{
    switch (c)
    {
        case '<': fputs("&lt;",  stream); break;
        case '>': fputs("&gt;",  stream); break;
        case '&': fputs("&amp;", stream); break;
        default:  fputc(c,       stream); break;
    }
}


void htmlize(FILE *in, FILE *out)
/*
 * TODO: Remove the redundant '\'s that are used to escape something
 * for e.g, right now,
 *       \<tag> -> \&lt;tag&gt;
 * but it should have been,
 *       \<tag> -> &lt;tag&gt;
 */
/*
 * Yet to be implemented -
 *  - &#...;  numeric character references
 *  - Links
 */
/*
 * Things implemented -
 *  - ```
 *  - `code`
 *  - *bold*
 *  - _italic_
 *  - <table>
 *  - # Headings
 *  - Lists
 *  - HTML <tags>
 *  - Linebreak if two spaces at line end
 *  - <br> at blank lines
 */
{
    char line[MAX_LINE_LENGTH];
    char last_line[MAX_LINE_LENGTH];

    unsigned char BOLD_OPEN = 0;
    unsigned char ITALIC_OPEN = 0;
    unsigned char CODE_OPEN = 0;
    unsigned char CODEBLOCK_OPEN = 0;
    unsigned char HTML_TAG_OPEN = 0;
    unsigned char TABLE_MODE = 0;
    unsigned char LIST_MODE = 0;

    unsigned int H_LEVEL = 0;

    char *pch;  // Previous char
    char *cch;  // Current char
    char *nch;  // Next char

    for (;;)
    {
        // Save current line in last_line
        strcpy(last_line, line);

        /*
         * Read and store a line from *in into line[]
         * Break loop if we've reached EOF (i.e. fgets() == NULL)
         */
        if (fgets(line, MAX_LINE_LENGTH, in) == NULL)
            break;


        // Blank lines
        if (!memcmp(line, "\n", 2))
        {
            if (!memcmp(last_line, "\n", 2))    // Previous line was blank
                fputs("<br>\n", out);
            else
                fputs("<br><br>\n", out);
            continue;
        }


        // For <pre> blocks
        if (!memcmp(line, "```", 3))
        {
            if (++CODEBLOCK_OPEN % 2)
                fputs("<pre>\n", out);
            else
                fputs("</pre>\n", out);
            continue;
        }
        if (CODEBLOCK_OPEN)
        {
            // Blindly escape everything and move on
            fputc_escaped(*cch, out);
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
                /* Check if the found '-' is the first char of the line */
                char *i = p;
                while (i > line)
                    if (!isblank(*--i))
                        break;
                if (i == line)
                {
                    memmove(&p[3], p, MAX_LINE_LENGTH-(p-line)-3);
                    *p++ = '<';
                    *p++ = 'l';
                    *p++ = 'i';
                    *p++ = '>';
                    while (*p == ' ')
                        memmove(p, &p[1], MAX_LINE_LENGTH-(p-line));
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
            H_LEVEL = 1;
            while (line[H_LEVEL++] == '#') {;}

            // Remove leading #'s and spaces
            memmove(&line[0], &line[H_LEVEL - 1], MAX_LINE_LENGTH);  // #'s
            while (line[0] == ' ')                                   // spaces
                memmove(&line[0], &line[1], MAX_LINE_LENGTH);

            // Create an ID for the heading
            char h_id[MAX_LINE_LENGTH] = "";
            for (int i=0,j=0; i < MAX_LINE_LENGTH; i++)
                if (line[i] == '\0')
                    break;
                else
                    if (line[i] == ' ')
                        h_id[j++] = '-';
                    else
                        if (isalnum(line[i]))
                            h_id[j++] = line[i];

            fprintf(out, "<h%i id=\"%s\">", H_LEVEL, h_id);
        }


        // Line prefix for tables
        if (TABLE_MODE)
            fputs("<tr><td>", out);


        /* iterate over the stored line[] */
        pch = &line[0];
        cch = &line[0];
        nch = &line[0];
        for (int index = 1; index < MAX_LINE_LENGTH; index++)
        {
            pch = cch;
            cch = nch;
            nch = &line[index];

            if (*cch == '\0')   // we've reached end of string
                break;          // stop iterating


            // Heading tag close
            if (H_LEVEL && *cch == '\n')
            {
                fprintf(out, "</h%u>\n", H_LEVEL);
                H_LEVEL = 0;
                continue;
            }

            // Linebreak if two spaces at line end
            if (*cch == '\n' && *pch == ' ' && line[index - 2] == ' ')
                fputs("<br>\n", out);


            // `code`
            if (*cch == '`' && !(isalnum(*pch) && isalnum(*nch)) && *pch != '\\')
            {
                if (CODE_OPEN = ++CODE_OPEN % 2)
                    fputs("<code>", out);
                else
                    fputs("</code>", out);
                continue;
            }


            if (!CODE_OPEN)
            {
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


                // *bold*
                if (*cch == '*' && !(isalnum(*pch) && isalnum(*nch)) && *pch != '\\')
                {
                    if (BOLD_OPEN = ++BOLD_OPEN % 2)
                        fputs("<b>", out);
                    else
                        fputs("</b>", out);
                    continue;
                }


                // _italic_
                if (*cch == '_' && !(isalnum(*pch) && isalnum(*nch)) && *pch != '\\')
                {
                    if (ITALIC_OPEN = ++ITALIC_OPEN % 2)
                        fputs("<i>", out);
                    else
                        fputs("</i>", out);
                    continue;
                }


                // Table cells
                if (TABLE_MODE)
                {
                    if (*cch == '|' && *pch != '\\')
                    {
                        fputs("</td><td>", out);
                        continue;
                    }
                    if (*cch == '\n')
                    {
                        fputs("</td></tr>\n", out);
                        continue;
                    }
                }
            }


            fputc_escaped(*cch, out);
        }
    }
}


int main(void)
{
    /*
     * Right now, it htmlize()'s stdin, and prints it to stdout
     * In the future, we shall htmlize() files from src/ to docs/
     */
    htmlize(stdin, stdout);
}

// vim:et:ts=4:sts=0:sw=0:fdm=syntax
