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
#include "include/date.h"
#include "include/stoi.h"
#include "constants.h"


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


char *urlencode_c(const char c, char *str_size_4)
{
    /* unreserved */
    if (isalnum(c))
    {
        sprintf(str_size_4, "%c", c);
        return str_size_4;
    }
    switch (c)
    {
        case '-':
        case '.':
        case '_':
        case '~':
            sprintf(str_size_4, "%c", c);
            return str_size_4;
        default:
            break;
    }

    /* reserved */
    sprintf(str_size_4, "%%%0X", c);
    return str_size_4;
}
char *urlencode_s(const char *s, char *storage)
{
    char str[4];
    char *pointer;
    pointer = storage;
    for (int i=0; s[i]!='\0' && i<=FILENAME_MAX*4; i++)
    {
        urlencode_c(s[i], str);
        memmove(pointer, str, strlen(str));
        pointer += strlen(str);
    }
    *pointer = '\0';
    return storage;
}


int main(void)
{
    DIR *dir;
    FILE *outfile;
    struct dirent *dirent;
    const char empty[] = {'\0'};
    const char *filenames[MAX_FILES + 1];  // +1 because indexing starts at 0

    // Mark all records as empty
    for (int i=1; i <= MAX_FILES; i++)
        filenames[i] = empty;

    if (cd(DEST_DIR))
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


    FILE *fp;
    char line[MAX_LINE_LENGTH];
    char TITLE[MAX_LINE_LENGTH];
    char DATE_CREATED[MAX_LINE_LENGTH];
    for (int i=1; filenames[i]!=empty && i<=MAX_FILES ; i++)
    {
        char filename[FILENAME_MAX + 1];
        memcpy(filename, filenames[i], FILENAME_MAX + 1);
        fp = fopen(filename, "r");
        if (fp == NULL)
            continue;
        fgets(line, MAX_LINE_LENGTH, fp);                   // <!--\n

        memmove(TITLE, fgets(line, MAX_LINE_LENGTH, fp), MAX_LINE_LENGTH);
        memmove(TITLE, TITLE + 6, MAX_LINE_LENGTH - 6);         // TITLE:
        while (*TITLE == ' ')
            memmove(TITLE, TITLE + 1, MAX_LINE_LENGTH - 1);
        *(strrchr(TITLE, '\n')) = '\0';

        memmove(DATE_CREATED, fgets(line, MAX_LINE_LENGTH, fp), MAX_LINE_LENGTH);
        memmove(DATE_CREATED, DATE_CREATED + 9, MAX_LINE_LENGTH - 9);  // DATE_CREATED:
        while (*DATE_CREATED == ' ')
            memmove(DATE_CREATED, TITLE + 1, MAX_LINE_LENGTH - 1);

        fclose(fp);


        char url[FILENAME_MAX*3 + 1];
        urlencode_s(filenames[i], url);
        fprintf(outfile,
                "<tr>\n"
                "    <td class=\"blog-index-name\">\n"
                "        <a href=\"%s\">%s</a>\n"
                "    </td>\n"
                "    <td class=\"blog-index-date\">\n"
                "        %s\n"
                "    </td>\n"
                "</tr>\n",
                url,
                TITLE,
                date_to_text(DATE_CREATED, 1)
             );
    }

    fputs(FINAL_TEXT, outfile);
    fclose(outfile);

    return 0;
}

// vim:et:ts=4:sts=0:sw=0:fdm=syntax
