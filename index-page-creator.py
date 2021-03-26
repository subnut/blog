"""
Simply recurses through the files in the docs directory, and makes an
index html that indexes all of them, by date (newest first; i.e. simply
use the filename - start from the biggest number and end at 1, NOT 0!)

By 'indexes all of them', I mean it creates a list of links. The link
text is the Title of that post, and the link points to, _obviously_, the
blog post.
"""
import html
import os
import sys

DIR = "docs"


def main():
    filenames = os.listdir(DIR)
    files = {int(file.split("-")[0]): file for file in filenames}
    filenumbers = list(files)
    filenumbers.sort(reverse=True)

    os.chdir(DIR)
    out_file = open("index.html", "w")
    print = lambda x: out_file.write(x + "\n")
    print(INITIAL_TEXT)

    for filenumber in filenumbers:
        filename = files[filenumber]
        print(f'<li><a href="{html.escape(filename)}">{html.escape(filename)}</a></li>')

    print(FINAL_TEXT)


INITIAL_TEXT = f"""\
<html>
    <head>
        <title>{html.escape("subnut's blog")}</title>
        <link rel="stylesheet" href="style.css">
    </head>
    <body>
        <h1 class="title">{html.escape("subnut's blog")}</h1>
        <ul>
<!-- Automated links start here -->"""

FINAL_TEXT = """\
<!-- Automated links stop here -->
        </ul>
    </body>
</head>"""

if __name__ == "__main__":
    os.chdir(os.path.dirname(os.path.abspath(sys.argv[0])))
    main()
