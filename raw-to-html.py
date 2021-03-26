import html
import sys
import os

SOURCE_DIR = "src"
DEST_DIR = "docs"


def main():
    def print(*x, **y):
        global print
        print(*x, **y, end="")

    for filename in os.listdir(SOURCE_DIR):
        os.chdir(SOURCE_DIR)
        with open(filename) as file:
            raw_lines = file.readlines()
        os.chdir("..")
        print(filename, "-> ")

        TITLE = raw_lines[1].rstrip("\n")
        DATE_CREATED = raw_lines[2].rstrip("\n")
        DATE_MODIFIED = raw_lines[3].rstrip("\n")

        raw_lines = raw_lines[5:]
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
                <td class="blog-date">{DATE_CREATED}</td>
            </tr><tr>
                <td class="blog-date">Last modified</td>
                <td class="blog-date">{DATE_MODIFIED}</td>
        </tr></table>
        <br>
<!-- Blog content starts here -->
"""


def htmlize(lines) -> str:
    OUTPUT = []
    print = lambda x: OUTPUT.append(x)

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
        if line.startswith("<table") and not TABLE_MODE:
            TABLE_MODE = True
            print(line)
            continue

        # Table end
        if line == "</table>\n" and TABLE_MODE:
            TABLE_MODE = False
            print(line)
            continue

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
            if not CODEBLOCK_OPEN:
                # Whether the '<' is start of an HTML tag
                if (
                    char == "<"
                    and not HTML_TAG_OPEN
                    and (line[index + 1] == "/" or line[index + 1].isalpha())
                ):
                    if line[index - 1] != "\\":
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
                        print("<br>\n")
                        continue

                    if char in "*_`":
                        if line[index - 1] == "\\":
                            OUTPUT.pop()
                            print(char)
                            continue
                        char_dict = char_DICT[char]
                        if char_dict["open"]:
                            if line[index + 1].isspace():
                                char_dict["open"] = False
                                print(f"</{char_dict['tag']}>")
                                continue
                        else:
                            if index == 0 or line[index - 1].isspace():
                                char_dict["open"] = True
                                print(f"<{char_dict['tag']}>")
                                continue
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
                            if line[index - 1] == "\\":
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
    main()
