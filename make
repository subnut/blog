#!/bin/sh
cd "$(dirname "$0")"
run() { echo "$@"; "$@"; }

if [ -n "$*" ]; then
  export "$@"
fi

if [ -z "$CC" ]; then
  CC=cc
fi

run "$CC" -Wall -O3 -static $CFLAGS $LDFLAGS -o index index.c
run "$CC" -Wall -O3 -static $CFLAGS $LDFLAGS -o htmlize htmlize.c

# vim:ft=sh:et:ts=2:sts=0:sw=0
