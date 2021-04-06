#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define MAX_LENGTH 500

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
{
    char line[MAX_LENGTH];

	char BOLD_OPEN = 0;
	char ITALIC_OPEN = 0;
	char CODE_OPEN = 0;
    char CODEBLOCK_OPEN = 0;
    char HTML_TAG_OPEN = 0;
    char TABLE_MODE = 0;

    char *pch;  // Previous char
    char *cch;  // Current char
    char *nch;  // Next char

    for (;;)
    {
        /*
         * Read and store a line from *in into line[]
         * Break loop if we've reached EOF (i.e. fgets() == NULL)
         */
        if (fgets(line, MAX_LENGTH, in) == NULL)
            break;


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


        // For <table>
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


        // Line prefix for tables
        if (TABLE_MODE)
            fputs("<tr><td>", out);


        /* iterate over the stored line[] */
        pch = &line[0];
        cch = &line[0];
        nch = &line[0];
        for (int index = 1; index < MAX_LENGTH; index++)
        {
            pch = cch;
            cch = nch;
            nch = &line[index];

            if (*cch == '\0')   // we've reached end of string
                break;          // stop iterating


            // `code`
            if (*cch == '`' && !(isalnum(*pch) && isalnum(*nch)) && *pch != '\\')
            {
                if (++CODE_OPEN % 2)
                    fputs("<code>\n", out);
                else
                    fputs("</code>\n", out);
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
                    if (++BOLD_OPEN % 2)
                        fputs("<b>", out);
                    else
                        fputs("</b>", out);
                    continue;
                }


                // _italic_
                if (*cch == '_' && !(isalnum(*pch) && isalnum(*nch)) && *pch != '\\')
                {
                    if (++ITALIC_OPEN % 2)
                        fputs("<i>", out);
                    else
                        fputs("</i>", out);
                    continue;
                }


                // Table cells
                if (TABLE_MODE)
                {
                    if (*cch == '|' && *pch != '\\')
                        fputs("</td><td>", out);
                    if (*cch == '\n')
                        fputs("</td></tr>\n", out);
                    continue;
                }
            }


            fputc_escaped(*cch, out);
        }
    }
}

int main(void)
{
    int i = 0;
    htmlize(stdin, stdout);
    printf("%i", ++i%2);
}

// vim:et:ts=4:sts=0:sw=0
