#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

/*
 * ctype.h  - isalnum
 * stdio.h  - fopen, fclose
 * string.h - memcmp, memchr
 * dirent.h - opendir, readdir
 */

#include "include/cd.h"
#include "include/stoi.h"

#define MAX_FILES 100
#define DIRECTORY "docs"

const char INITIAL_TEXT[] = "\
<html>\n\
    <head>\n\
        <title>subnut's blog</title>\n\
        <link rel=\"stylesheet\" href=\"style.css\">\n\
    </head>\n\
    <body class=\"blog-index\">\n\
        <h1 class=\"title\">subnut's blog</h1>\n\
        <table class=\"blog-index\">\n\
<!-- Automation starts here -->\n\
";
const char FINAL_TEXT[] = "\
<!-- Automation stops here -->\n\
        </table>\n\
    </body>\n\
</head>\n\
";


void fputc_urlencoded(int c, FILE *stream)
{
    /* unreserved */
    if (isalnum(c))
    {
        fputc(c, stream);
        return;
    }
    switch (c)
    {
        case '-':
        case '.':
        case '_':
        case '~':
            fputc(c, stream);
            return;
    }

    /* reserved */
    fprintf(stream, "%%%0X", c);
    return;
}
void fputs_urlencoded(const char *s, FILE *stream)
{
    for (int i=0; s[i] != '\0'; i++)
        fputc_urlencoded(s[i], stream);
}


int main(void)
{
    DIR *dir;
    struct dirent *dirent;
    char *filenames[MAX_FILES + 1];  // +1 because indexing starts at 0
    FILE *outfile;

    // Mark empty records as '\0'
    for (int i=1; i <= MAX_FILES; i++)
        filenames[i] = '\0';

    if (cd(DIRECTORY))
        return 1;

    outfile = fopen("index.html", "w");
    fputs(INITIAL_TEXT, outfile);

    if ((dir = opendir(".")) == NULL)
    {
        fprintf(stderr, "index: opendir error");
        return 1;
    }
    while ((dirent = readdir(dir)) != NULL)
    {
        char *name = &(*dirent->d_name);
        if (
                name[0] == '.'      // ie. ., .., and hidden files
                || name[0] == '0'   // ie. 0-draft, 0-format, etc.
                || memcmp(strrchr(name, '.'), ".html", 5)   // not *.html
                || !memcmp(name, "index.html", 10)
           )
            continue;

        char *p = memchr(name, '-', FILENAME_MAX);  // defined in stdio.h
        *p = '\0';

        int num = stoi(name);
        *p = '-';

        filenames[num] = name;
    }
    closedir(dir);

    for (int i=1; filenames[i] != '\0'; i++)
    {

        fputs(
"<tr>
    <td class=\"blog-index-name\"><a href=\"", outfile);

        fputs_urlencoded(filenames[i], outfile);

        fputs(                              "\">{TITLE}</a></td>
    <td class=\"blog-index-date\">{date_to_text(CREATED)}</td>
</tr>
", outfile);

    }

    fputs(FINAL_TEXT, outfile);
    fclose(outfile);

    return 0;
}

// vim:et:ts=4:sts=0:sw=0:fdm=syntax
