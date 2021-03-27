#!/bin/sh
if [ `git status --porcelain | wc -l` != 0 ]; then
	echo 'dirty working tree' >&2
	echo 'please stash/commit your changes first' >&2
	exit 1
fi
run() { echo "> \"$@\""; "$@"; }
run git branch -D gh-pages
run git checkout -b gh-pages
run mkdir docs
cp style.css docs
run python raw-to-html.py
run python index-page-creator.py
run git add -f docs
run git commit -m PUBLISH
run trap : INT
run git push --set-upstream $(git remote) gh-pages --force
run git switch -
