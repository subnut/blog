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

#define date_to_text(x)     date_to_text(x, 1)
#define cd(x)               cd(x, argv)

const char INITIAL_TEXT[] = "\
<html>\n\
    <head>\n\
        <meta charset=\"utf-8\"/>\n\
        <title>subnut's blog</title>\n\
        %s\n\
        <link rel=\"stylesheet\" href=\"style.css\">\n\
<!--    <link rel=\"stylesheet\" href=\"recursive.css\">  -->\n\
    </head>\n\
    <body class=\"blog-index\">\n\
        <h1 class=\"title\">subnut's blog</h1>\n\
        <table class=\"blog-index\">\n\
<!-- Index starts here -->\n\
";
const char FINAL_TEXT[] = "\
<!-- Index ends here -->\n\
        </table>\n\
        <br>\n\
        <br>\n\
        %s\n\
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


int main(int argc, const char **argv)
{
	DIR *dir;
	FILE *outfile;
	struct dirent *dirent;
	char filenames[MAX_FILES + 1][FILENAME_MAX + 1];	// +1 because index starts at 0

	// Mark all records as empty
	for (int i=1; i <= MAX_FILES; i++)
		*filenames[i] = '\0';

	if (cd(DEST_DIR))
		return 1;

	outfile = fopen("index.html", "w");
	fprintf(outfile, INITIAL_TEXT, FAVICON);

	if ((dir = opendir(".")) == NULL)
	{
		fprintf(stderr, "index: opendir error");
		return 1;
	}
	while ((dirent = readdir(dir)) != NULL)
	{
		char *name = dirent->d_name;
		if (
				name[0] == '.'		// ie. ., .., and hidden files
				|| name[0] == '0'	// ie. 0-draft, 0-format, etc.
				|| memcmp(strrchr(name, '.'), ".html", 5)	// not *.html
				|| !memcmp(name, "index.html", 10)
		   )
			continue;

		char *p = memchr(name, '-', FILENAME_MAX);	// defined in stdio.h
		*p = '\0';

		int num = stoi(name);
		*p = '-';

		strcpy(filenames[num], name);
	}
	closedir(dir);


	FILE *fp;
	char line[MAX_LINE_LENGTH];
	char TITLE[MAX_LINE_LENGTH];
	char DATE_CREATED[MAX_LINE_LENGTH];
	for (int i=1; *filenames[i]!='\0' && i<=MAX_FILES ; i++)
	{
		if ((fp = fopen(filenames[i], "r")) == NULL)
			continue;
		fgets(line, MAX_LINE_LENGTH, fp);					// <!--\n

		memmove(TITLE, fgets(line, MAX_LINE_LENGTH, fp), MAX_LINE_LENGTH);
		memmove(TITLE, TITLE + 6, MAX_LINE_LENGTH - 6);		// TITLE:
		while (*TITLE == ' ')
			memmove(TITLE, TITLE + 1, MAX_LINE_LENGTH - 1);
		*(strrchr(TITLE, '\n')) = '\0';

		memmove(DATE_CREATED, fgets(line, MAX_LINE_LENGTH, fp), MAX_LINE_LENGTH);
		memmove(DATE_CREATED, DATE_CREATED + 9, MAX_LINE_LENGTH - 9);	// DATE_CREATED:
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
				date_to_text(DATE_CREATED)
			 );

#ifdef PRINT_FILENAMES
		printf("%i:\t%s\n", i, filenames[i]);
#endif

	}

	fprintf(outfile, FINAL_TEXT, FOOTER);
	fclose(outfile);

	return 0;
}

// vim:noet:ts=4:sts=0:sw=0:fdm=syntax
