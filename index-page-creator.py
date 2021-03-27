#!/usr/bin/python3

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
import urllib.parse

DIR = "docs"


def main():
    filenames = os.listdir(DIR)
    index = filenames.__len__()
    while index:
        index -= 1
        filename = filenames[index]
        if filename in ("index.html", "style.css") or filename[0] == "0":
            del filenames[index]
    files = {int(file.split("-", 1)[0]): file for file in filenames}
    filenumbers = list(files)
    filenumbers.sort(reverse=True)

    os.chdir(DIR)
    out_file = open("index.html", "w")
    print = lambda x: out_file.writelines([x, "\n"])
    print(INITIAL_TEXT)

    for filenumber in filenumbers:
        filename = files[filenumber]
        with open(filename) as file:
            file.readline()
            TITLE = file.readline()[6:-1]
            CREATED = file.readline()[8:-1]
        print(
            f"""\
<tr>
    <td class="blog-index-name"><a href="{urllib.parse.quote(filename)}">{TITLE}</a></td>
    <td class="blog-index-date">{date_to_text(CREATED)}</td>
</tr>"""
        )

    print(FINAL_TEXT)


def date_to_text(date: str) -> str:
    OUTPUT = []
    YYYY, MM, DD = date.split("/")

    # Date
    DD = int(DD)
    if DD in (1, 2, 3):
        DD = {
            1: "1st",
            2: "2nd",
            3: "3rd",
        }[DD]
    else:
        DD = f"{DD}th"
    OUTPUT.append(DD)

    # Month
    MM = int(MM)
    MM = {
        1: "January",
        2: "February",
        3: "March",
        4: "April",
        5: "May",
        6: "June",
        7: "July",
        8: "August",
        9: "September",
        10: "October",
        11: "November",
        12: "December",
    }[MM]
    OUTPUT.append(MM)

    # Year
    OUTPUT.append(YYYY)

    # OUTPUT
    OUTPUT = " ".join(OUTPUT)
    return OUTPUT


INITIAL_TEXT = f"""\
<html>
    <head>
        <title>{html.escape("subnut's blog")}</title>
        <link rel="stylesheet" href="style.css">
    </head>
    <body class="blog-index">
        <h1 class="title">subnut{html.escape("'")}s blog</h1>
        <table class="blog-index">
<!-- Automation starts here -->"""

FINAL_TEXT = """\
<!-- Automation stops here -->
        </table>
    </body>
</head>"""

if __name__ == "__main__":
    os.chdir(os.path.dirname(os.path.abspath(sys.argv[0])))
    main()
