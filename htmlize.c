#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/*
 * ctype.h  - isalnum(), isalpha(), etc.
 * dirent.h - opendir(), readdir()
 * stdio.h  - printf(), fopen(), fprintf(), etc
 * string.h - str*(), mem*()
 */

#include "include/cd.h"
#include "include/date.h"
#include "include/stoi.h"
#include "constants.h"

#define date_to_text(x)     date_to_text(x, 0)
#define cd(x)               cd(x, argv)

const char INITIAL_HTML_PRE_SUBTITLE[] = "\
<html>\n\
    <head>\n\
        <title>%s</title>\n\
        <link rel=\"stylesheet\" href=\"style.css\">\n\
    </head>\n\
    <body>\n\
        <h1 class=\"title\">%s</h1>\n\
        <span class=\"subtitle\">\n\
";
const char INITIAL_HTML_POST_SUBTITLE[] = "\
        </span>\n\
        <table class=\"blog-date\"><tr>\n\
                <td class=\"blog-date\">Date created</td>\n\
                <td class=\"blog-date\">%s</td>\n\
            </tr><tr>\n\
                <td class=\"blog-date\">Last modified</td>\n\
                <td class=\"blog-date\">%s</td>\n\
        </tr></table>\n\
<!-- Blog content starts here -->\n\
";


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
void fputs_escaped(const char *s, FILE *stream)
{
    for (int i=0; s[i] != '\0'; i++)
        fputc_escaped(s[i], stream);
}


void htmlize(FILE *in, FILE *out)
/*
 * Things that are escaped using '\\' -
 *  - \`code\`
 *  - \*bold\*
 *  - \_italic\_
 *  - \<HTML>
 *  - Table \| cells
 *  - \&#...; HTML Numeric char ref
 *  - \!(ID)[Link text\]
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
 *  - &#...;  numeric character references
 *  - <br> at blank lines
 *  - Links
 */
{
    char TITLE[MAX_LINE_LENGTH];
    char DATE_CREATED[MAX_LINE_LENGTH];
    char DATE_MODIFIED[MAX_LINE_LENGTH];

    char line[MAX_LINE_LENGTH];
    char last_line[MAX_LINE_LENGTH];
    char links[MAX_LINKS + 1][MAX_LINE_LENGTH];  // +1 because indexing starts at 0


    /* Collect links */
    while (fgets(line, MAX_LINE_LENGTH, in) != NULL)
    {
        if (!memcmp(line, "![", 2))     // i.e. line starts with ![
        {
            char *p = memchr(line, ']', MAX_LINE_LENGTH);
            *p = '\0';

            int i = 2;
            while (p[i] == ' ')
                i++;

            char link[MAX_LINE_LENGTH];
            memmove(link, p + i, MAX_LINE_LENGTH - (p-line + i));
            *(strrchr(link, '\n')) = '\0';

            int link_id = stoi(line + 2);
            memmove(links[link_id], link, strlen(link)+1);  // +1 for '\0'
        }
    }
    rewind(in);


    /* ---- BEGIN initial HTML ---- */
    fgets(line, MAX_LINE_LENGTH, in);   // ---\n
    memmove(TITLE,         fgets(line, MAX_LINE_LENGTH, in), MAX_LINE_LENGTH);
    memmove(DATE_CREATED,  fgets(line, MAX_LINE_LENGTH, in), MAX_LINE_LENGTH);
    memmove(DATE_MODIFIED, fgets(line, MAX_LINE_LENGTH, in), MAX_LINE_LENGTH);
    fgets(line, MAX_LINE_LENGTH, in);   // ---\n

    fputs("<!--\n", out);
    fprintf(out, "TITLE: %s", TITLE);
    fprintf(out, "CREATED: %s", DATE_CREATED);
    fprintf(out, "MODIFIED: %s", DATE_MODIFIED);
    fputs("-->\n", out);

    char *p;
    *(p = memchr(TITLE,         '\n', MAX_LINE_LENGTH)) = '\0';
    *(p = memchr(DATE_CREATED,  '\n', MAX_LINE_LENGTH)) = '\0';
    *(p = memchr(DATE_MODIFIED, '\n', MAX_LINE_LENGTH)) = '\0';

    fprintf(out, INITIAL_HTML_PRE_SUBTITLE, TITLE, TITLE);
    while (strcmp(fgets(line, MAX_LINE_LENGTH, in), "---\n"))  // ie. line != "---\n"
        fputs_escaped(line, out);
    fprintf(out, INITIAL_HTML_POST_SUBTITLE,
            date_to_text(DATE_CREATED),
            date_to_text(DATE_MODIFIED)
           );
    /* ---- END initial HTML ---- */


    unsigned char BOLD_OPEN = 0;
    unsigned char ITALIC_OPEN = 0;
    unsigned char CODE_OPEN = 0;
    unsigned char CODEBLOCK_OPEN = 0;
    unsigned char HTML_TAG_OPEN = 0;
    unsigned char LINK_OPEN = 0;
    unsigned char LINK_TEXT_OPEN = 0;
    unsigned char TABLE_MODE = 0;
    unsigned char LIST_MODE = 0;

    unsigned int H_LEVEL = 0;

    char pch;  // Previous char
    char cch;  // Current char
    char nch;  // Next char

    // Initialize them now to avoid undefined behaviour
    pch = '\0';
    cch = '\0';
    nch = '\0';

    for (;;)
    {
        // Save current line in last_line
        memmove(last_line, line, MAX_LINE_LENGTH);

        /*
         * Read and store a line from *in into line[]
         * Break loop if we've reached EOF (i.e. fgets() == NULL)
         */
        if (fgets(line, MAX_LINE_LENGTH, in) == NULL)
            break;

        /*
         * NOTE: Always remember, CODEBLOCKs take precedence over anything else
         */

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
            fputc_escaped(cch, out);
            continue;
        }


        // End of blog
        if (!memcmp(line, "---\n", 4))
            break;

        // Link definitions
        if (!memcmp(line, "![", 2))
            continue;

        // Blank lines
        if (!memcmp(line, "\n", 2))
        {
            if (!memcmp(last_line, "\n", 2))     // Previous line was blank
                fputs("<br>\n", out);
            else
                if (memcmp(last_line, "![", 2))     // Prev line wasn't link defn
                    if (memcmp(last_line, "<", 1))  // Prev line (hopefully) wasn't HTML tag
                        fputs("<br><br>\n", out);
            continue;

            /*
             * For convenience, if there is a blank line after the link
             * definition(s), then we shall not treat it as a blank line
             *
             * e.g. -
             * │
             * │lorem ipsum dolor sit amet.
             * │
             * │![1]: https://localhost
             * │![2]: http://localhost
             * │
             * │Lorem Ipsum Dolor sit Amet
             * │consecteiur blah blah blah
             * │
             *
             * Shall be interpreted as -
             * │
             * │lorem ipsum dolor sit amet.
             * │
             * │Lorem Ipsum Dolor sit Amet
             * │consecteiur blah blah blah
             * │
             *
             * Which... is obviously what the original text would have been
             * if the links weren't there.
             */
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
            memmove(&line[0], &line[H_LEVEL - 1], MAX_LINE_LENGTH - (H_LEVEL - 1));  // #'s
            while (line[0] == ' ')                                   // spaces
                memmove(line, line + 1, MAX_LINE_LENGTH - 1);

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
        pch = *line;
        cch = *line;
        nch = *line;
        for (int index = 1; index < MAX_LINE_LENGTH; index++)
        {
            pch = cch;
            cch = nch;
            nch = *(line + index);

            if (cch == '\0')   // we've reached end of string
                break;          // stop iterating


            if (cch == '\\')
            {
                if (
                        (nch == '`' || nch == '*' || nch == '_')
                        || (!HTML_TAG_OPEN && nch == '<')
                        || (TABLE_MODE && nch == '|')
                        || (nch == '&' && line[index + 1] == '#')
                        || (!LINK_OPEN && nch == '!' && line[index + 1] == '(')
                   )
                {;}
                else
                    fputc('\\', out);
                continue;
            }


            // Heading tag close
            if (H_LEVEL && cch == '\n')
            {
                fprintf(out, "</h%u>\n", H_LEVEL);
                H_LEVEL = 0;
                continue;
            }


            /*
             * NOTE: As I NOTE'd above, CODE shall always come first
             */

            // `code`
            if (!CODE_OPEN && cch == '`' && !(isalnum(pch) && isalnum(nch)) && pch != '\\')
            {
                CODE_OPEN = 1;
                fputs("<code>", out);
                continue;
            }
            if (CODE_OPEN && cch == '`' && pch != '\\')
            {
                CODE_OPEN = 0;
                fputs("<code>", out);
                continue;
            }
            if (CODE_OPEN)
            {
                // Nothing to see here, just escape and move along
                fputc_escaped(cch, out);
                continue;
            }


            // Linebreak if two spaces at line end
            if (cch == '\n' && pch == ' ' && line[index - 2] == ' ')
                fputs("<br>\n", out);


            // HTML <tags>
            if ((cch == '<') && (nch == '/' || isalpha(nch)) && pch != '\\')
            {
                HTML_TAG_OPEN = 1;
                fputc('<', out);
                continue;
            }
            if (HTML_TAG_OPEN && cch == '>')
            {
                HTML_TAG_OPEN = 0;
                fputc('>', out);
                continue;
            }
            if (HTML_TAG_OPEN)
            {
                // Nothing needs escaping, fputc() and move on
                fputc(cch, out);
                continue;
            }


            // HTML Numeric Character references
            if (cch == '&' && nch == '#')
            {
                if (pch == '\\')
                    fputs("&amp;", out);
                else
                    fputc('&', out);
                continue;
            }


            // *bold*
            if (cch == '*' && !(isalnum(pch) && isalnum(nch)) && pch != '\\')
            {
                ++BOLD_OPEN;
                if (BOLD_OPEN %= 2)
                    fputs("<b>", out);
                else
                    fputs("</b>", out);
                continue;
            }


            // _italic_
            if (cch == '_' && !(isalnum(pch) && isalnum(nch)) && pch != '\\')
            {
                ++ITALIC_OPEN;
                if (ITALIC_OPEN %= 2)
                    fputs("<i>", out);
                else
                    fputs("</i>", out);
                continue;
            }


            // Links    !(ID) attribute="value" [text]
            if (cch == '!' && nch == '(' && pch != '\\' && !LINK_OPEN)
            {
                LINK_OPEN = 1;
                fputs("<a href=\"", out);

                char *p = memchr(line+index, ')', MAX_LINE_LENGTH - index);
                *p = '\0';

                int link_id = stoi(line + index + 1);
                fputs(links[link_id], out);

                *p = ')';
                nch = ')';
                index = p - line;
                continue;
            }
            if (LINK_OPEN)
            {
                if (cch == ')' || cch == '[')
                    if (cch == ')')
                        fputc('"', out);
                    else /* cch == '[' */
                    {
                        fputc('>', out);
                        LINK_OPEN = 0;
                        LINK_TEXT_OPEN = 1;
                    }
                else
                    fputc(cch, out);
                continue;
            }
            if (LINK_TEXT_OPEN && cch == ']' && nch != '\\')
            {
                LINK_TEXT_OPEN = 0;
                fputs("</a>", out);
                continue;
            }


            // Table cells
            if (TABLE_MODE)
            {
                if (cch == '|' && pch != '\\')
                {
                    fputs("</td><td>", out);
                    continue;
                }
                if (cch == '\n')
                {
                    fputs("</td></tr>\n", out);
                    continue;
                }
            }


            // It's just a simple, innocent character
            fputc_escaped(cch, out);
        }
    }


    fputs(
            "<!-- Blog content ends here -->\n"
            "    </body>\n"
            "</html>\n",
            out
         );
}


int main(int argc, const char **argv)
{
    DIR *dir;
    struct dirent *dirent;
    if ((dir = opendir(SOURCE_DIR)) == NULL)
    {
        switch (errno)
        {
            case ENOENT:
                fprintf(stderr, "htmlize: directory not found: %s\n", SOURCE_DIR);
                break;
            case ENOTDIR:
                fprintf(stderr, "htmlize: not a directory: %s\n", SOURCE_DIR);
                break;
            case EACCES:
                fprintf(stderr, "htmlize: permission denied: %s\n", SOURCE_DIR);
                break;
            default:
                fprintf(stderr, "htmlize: opendir error: %s\n", SOURCE_DIR);
                break;
        }
        return 1;
    }
    while ((dirent = readdir(dir)) != NULL)
    {
        char *name = &(*dirent->d_name);
        if (!memcmp(strrchr(name, '.'), SOURCE_EXT, strlen(SOURCE_EXT)))
        {
            int mem_needed = strlen(name) + 1 + (5-strlen(SOURCE_EXT));  // 5 for ".html"
            char new_name[mem_needed];
            memmove(new_name, name, mem_needed);
            char *p = strrchr(new_name, '.');
            1[p] = 'h';
            2[p] = 't';
            3[p] = 'm';
            4[p] = 'l';
            5[p] = '\0';

            if (cd(SOURCE_DIR))
                return 1;
            FILE *sfp = fopen(name, "r");
            cd("..");

            if (cd(DEST_DIR))
                return 1;
            FILE *dfp = fopen(new_name, "w");
            cd("..");

            htmlize(sfp, dfp);
            fclose(sfp);
            fclose(dfp);
            printf("%s -> %s\n", name, new_name);
        }
    }
    closedir(dir);
}

// vim:et:ts=4:sts=0:sw=0:fdm=syntax
