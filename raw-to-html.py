#!/usr/bin/python3

# Some speed tips -
# (taken using the `timeit` builtin module)
#
# ```
# import timeit
# x1=timeit.Timer('y,m,d = c.split("/")', "c='yyyy/mm/dd'")
# x2=timeit.Timer('y=x[:4];m=x[5:7];d=x[8:10]','x="YYYY/MM/DD\\n"')
# ```
#
# - Slicing is faster that "".startswith()
#     e.g. (x[2:] == "- ") is better than x.startswith("- ")
#   The same applies for .endswith()
#
# - `del list[index]` is faster than `list.pop(index)`
#   But, `list.pop()` is faster than `del list[-1]`
#
# - list.append(object) is faster than list + [object]
#   (Which is what we do. So that's good!)
#
# -

import html
import os
import sys

SOURCE_DIR = "src"
DEST_DIR = "docs"


def main():
    def print(*x, **y):
        global print
        print(*x, **y, end="")

    for filename in [x for x in os.listdir(SOURCE_DIR) if x[-4:] == ".raw"]:
        os.chdir(SOURCE_DIR)
        with open(filename) as file:
            raw_lines = file.readlines()
        os.chdir("..")
        print(filename, "-> ")

        del raw_lines[0]  # "---\n"
        TITLE = raw_lines.pop(0).rstrip("\n")
        DATE_CREATED = raw_lines.pop(0).rstrip("\n")
        DATE_MODIFIED = raw_lines.pop(0).rstrip("\n")
        del raw_lines[0]  # "---\n"

        SUBTITLE = "".join(raw_lines[: raw_lines.index("---\n")]).rstrip("\n")
        raw_lines = raw_lines[raw_lines.index("---\n") + 1 :]
        raw_lines = raw_lines[: raw_lines.index("---\n")]

        final_lines = []
        final_lines.append(initial_html(TITLE, SUBTITLE, DATE_CREATED, DATE_MODIFIED))
        final_lines.append(htmlize(raw_lines))
        final_lines.append(final_html())
        final_text = "".join(final_lines)

        os.chdir(DEST_DIR)
        filename = filename.replace(".raw", ".html")
        with open(filename, "w") as file:
            file.write(final_text)
        os.chdir("..")
        print(filename, "\n")


def date_to_text(date: str) -> str:
    OUTPUT = []

    # date.split() is faster than
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
        1: "Jan",
        2: "Feb",
        3: "Mar",
        4: "Apr",
        5: "May",
        6: "June",
        7: "July",
        8: "Aug",
        9: "Sept",
        10: "Oct",
        11: "Nov",
        12: "Dec",
    }[MM]
    OUTPUT.append(MM)

    # Year
    OUTPUT.append(YYYY)

    # OUTPUT
    OUTPUT = " ".join(OUTPUT)
    return OUTPUT


def final_html():
    return """\
<!-- Blog content ends here -->
    </body>
</html>
"""


def initial_html(TITLE, SUBTITLE, DATE_CREATED, DATE_MODIFIED):
    return f"""\
<html>
    <head>
        <title>{html.escape(TITLE)}</title>
        <link rel="stylesheet" href="style.css">
    </head>
    <body>
        <h1 class="title">{html.escape(TITLE)}</h1>
        <span class="subtitle">
{html.escape(SUBTITLE)}
        </span>
        <table class="blog-date"><tr>
                <td class="blog-date">Date created</td>
                <td class="blog-date">{date_to_text(DATE_CREATED)}</td>
            </tr><tr>
                <td class="blog-date">Last modified</td>
                <td class="blog-date">{date_to_text(DATE_MODIFIED)}</td>
        </tr></table>
        <br>
<!-- Blog content starts here -->
"""


def htmlize(lines) -> str:
    OUTPUT = []
    print = lambda x: OUTPUT.append(x)

    LIST_MODE = False
    TABLE_MODE = False
    CODEBLOCK_OPEN = False
    HTML_TAG_OPEN = False

    char_DICT = {
        "*": {"open": False, "tag": "b"},
        "_": {"open": False, "tag": "i"},
        "`": {"open": False, "tag": "code"},
    }

    for linenr, line in enumerate(lines):

        # Table
        if line[:6] == "<table" and not TABLE_MODE:
            TABLE_MODE = True
            print(line)
            continue

        # Table end
        if line == "</table>\n" and TABLE_MODE:
            TABLE_MODE = False
            print(line)
            continue

        # List
        if line[:3] in ("<ul", "<ol") and not LIST_MODE:
            LIST_MODE = True
            print(line)
            continue

        # List item
        if LIST_MODE and line.lstrip()[:2] == "- ":
            line = line.replace("- ", "<li>", 1)

        # List end
        if line in ("</ul>\n", "</ol>\n") and LIST_MODE:
            LIST_MODE = False
            print(line)
            continue

        # NOTE: Since we don't check indented tags, a clever hack to support
        # nested lists is to simply use proper indentation!
        # e.g. -
        # |<ul>
        # |- Item 1
        # |- Item 2
        # |  <ul>
        # |  - Item 2.1
        # |  - Item 2.2
        # |  - Item 2.3
        # |  </ul>
        # |- Item 3
        # |- Item 4
        # |</ul>

        # Blank lines
        if line == "\n" and not CODEBLOCK_OPEN:
            if lines[linenr - 1] != "\n":
                print("<br>")
            print("<br>\n")
            continue

        # Codeblocks using ```
        if line == "```\n":
            if CODEBLOCK_OPEN:
                CODEBLOCK_OPEN = False
                print("</pre>\n")
            else:
                CODEBLOCK_OPEN = True
                print("<pre>\n")
            continue

        # Headings using '#'
        H_LEVEL = 0
        if line[0] == "#":
            H_LEVEL = 2
            if line[1] == "#":
                H_LEVEL = 3
                if line[2] == "#":
                    H_LEVEL = 4
            print(f"<h{H_LEVEL}>")
            line = line[H_LEVEL - 1 :].strip()
            # line = f"<h{H_LEVEL}>{line[H_LEVEL-1:].strip()}</h{H_LEVEL}>\n"

        # Support for Tables
        if TABLE_MODE:
            print("<tr><td>")

        for index, char in enumerate(line):

            # Slice it once instead of slicing it every time we need it.
            # Should by faster, right? (Not tested yet)
            if index:
                prev_char = line[index - 1]
            if char != "\n":
                next_char = line[index + 1]

            if not CODEBLOCK_OPEN:
                # Whether the '<' is start of an HTML tag
                if (
                    char == "<"
                    and not HTML_TAG_OPEN
                    and (next_char == "/" or next_char.isalpha())
                ):
                    if prev_char != "\\":
                        HTML_TAG_OPEN = True
                        print(char)
                    else:
                        OUTPUT.pop()  # remove the '\' that escaped char
                        print(char)
                    continue

                # Whether the '>' is part of an HTML tag
                if char == ">" and HTML_TAG_OPEN:
                    HTML_TAG_OPEN = False
                    print(char)
                    continue

                if not HTML_TAG_OPEN:

                    # Linebreak if two spaces at line end
                    if char == "\n" and index > 1 and line[-3:-1] == "  ":
                        OUTPUT.pop()
                        OUTPUT.pop()
                        print("<br>\n")
                        continue

                    # *bold* _italic_ `code`,
                    # or anything else defined in char_DICT
                    if char in char_DICT:
                        # If char is preceded with backslash, we shall not
                        # consider it, no matter what.
                        if prev_char == "\\":
                            OUTPUT.pop()
                            print(char)
                            continue

                        char_dict = char_DICT[char]
                        if char_dict["open"]:
                            # If we're open _already_, we can close anywhere.
                            # No conditions needed.  Why this rule exists? See
                            # first sentence of this comment block!
                            # (Hint: _already_,)
                            char_dict["open"] = False
                            print(f"</{char_dict['tag']}>")

                        elif (
                            index == 0 or prev_char.isspace()
                        ) and next_char.isspace():
                            # If we're not open yet, then, to open, we mustn't
                            # be preceded by anything. This is to ensure that
                            # things like some_variable don't accidentally
                            # trigger italics.
                            # But, if the next char is also a space, then the
                            # char is just a stray. Spare it.
                            char_dict["open"] = True
                            print(f"<{char_dict['tag']}>")

                        else:
                            # It is what it is
                            print(char)

                        continue

                    # Support for tables
                    if TABLE_MODE:

                        char_DICT_any_char_open = False
                        for char_dict in char_DICT.values():
                            char_DICT_any_char_open = (
                                char_DICT_any_char_open or char_dict["open"]
                            )
                            # i.e. if any char of char_DICT is "open",
                            # char_DICT_any_char_open shall become True

                        if char == "|" and not char_DICT_any_char_open:
                            if prev_char == "\\":
                                OUTPUT.pop()
                                print(char)
                                continue
                            print("</td><td>")
                            continue

                        if char == "\n":
                            print("</td></tr>\n")
                            continue

                # We're inside an HTML_TAG. Don't escape.
                else:
                    print(char)
                    continue

            # It's a normal character. Escape it.
            print(html.escape(char))

        # If this line was a Heading
        if H_LEVEL:
            print(f"</h{H_LEVEL}>\n")

    return "".join(OUTPUT)


if __name__ == "__main__":
    os.chdir(os.path.dirname(os.path.abspath(sys.argv[0])))
    main()
