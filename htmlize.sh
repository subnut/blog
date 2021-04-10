#!/bin/sh

SOURCE_EXT=.raw
SOURCE_DIR=src
DEST_DIR=docs

if [ ! -d "$SOURCE_DIR" ]; then
    printf "%s: directory not found: %s\n" "$0" "$SOURCE_DIR"
    exit 1
fi
if [ ! -d "$DEST_DIR" ]; then
    printf "%s: directory not found: %s\n" "$0" "$DEST_DIR"
    exit 1
fi

function print_escaped_char
{
    case $1
        (&) echo -n '&amp;';;
        (<) echo -n '&lt;' ;;
        (>) echo -n '&gt;' ;;
        (*) echo -n $1     ;;
    esac
}
function print_escaped_string
{
    for char in $(echo -n "$1" | fold -w 1); do
        print_escaped_char $char
    done
}

cd "$SOURCE_DIR"
for FILENAME in *"$SOURCE_EXT"
do
    if [ ! -r "$FILENAME" ]; then
        continue
    fi

    DEST_FILENAME="${FILENAME%%$SOURCE_EXT}.html"
    DEST_FILENAME="../$DEST_DIR/$DEST_FILENAME"
    exec 1>"$DEST_FILENAME"

    CODEBLOCK_OPEN=0
    LIST_MODE=0

    LINE=""
    PREV_LINE=""
    cat "$FILENAME" | sed 's/\\/\\\\/g' | while IFS=$'\n' read -r LINE
    do
        if [ "$LINE" = "```\n"  ]; then
           if (( CODEBLOCK_OPEN = (CODEBLOCK_OPEN + 1) % 2)); then
               echo '<pre>'
           else
               echo '</pre>'
           fi
        fi
        if [ $CODEBLOCK_OPEN ]; then
            print_escaped_string "$LINE\n"
            continue
        fi

        if echo -n "$LINE" | grep '^<\(u\|o\)l'; then
            LIST_MODE=1;
        fi
        if echo -n "$LINE" | grep '^</\(u\|o\)l'; then
            LIST_MODE=0;
        fi

        PREV_LINE="$LINE"
    done
done



# vim:et ts=4 sts=0 sw=0
