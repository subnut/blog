#!/bin/sh
cd "$(dirname "$0")"
run() { echo "$@"; "$@"; }

if [ -n "$*" ]; then
  export "$@"
fi

if [ -z "$CC" ]; then
  CC=cc
fi

export CFLAGS="-Wall -O2 $CFLAGS"
export CPPFLAGS="-I.     $CPPFLAGS"

run "$CC" $CFLAGS $CPPFLAGS $LDFLAGS -c src/*
run "$CC" $CFLAGS $CPPFLAGS $LDFLAGS -o index   index.o   cd.o date_to_text.o stoi.o
run "$CC" $CFLAGS $CPPFLAGS $LDFLAGS -o convert convert.o cd.o date_to_text.o stoi.o htmlize.o
run "$CC" $CFLAGS $CPPFLAGS $LDFLAGS -o htmlize                date_to_text.o stoi.o htmlize.o \
-x c - << EOF
#include <stdio.h>
#include "include/htmlize.h"
int main(void) { htmlize(stdin, stdout); return 0; }
EOF
run rm *.o

# vim:ft=sh:et:ts=2:sts=0:sw=0
