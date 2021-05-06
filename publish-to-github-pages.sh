#!/bin/sh
if [ `git status --porcelain | wc -l` != 0 ]; then
	echo 'dirty working tree' >&2
	echo 'please stash/commit your changes first' >&2
	exit 1
fi
run() { echo "> \"$@\""; "$@"; }
run test -d raw || (echo "raw directory not found" >&2 && exit 1)
run git branch -D gh-pages
run git checkout -b gh-pages
run mkdir docs || (run rm docs -rv && run mkdir docs)
run convert
run index
run cp -v style.css     docs
run cp -v recursive.css docs
run cp -v print.css     docs
run cp -v MIT.txt       docs
# run cp -v src/* docs
run git add -f docs
run git commit -m PUBLISH
run trap : INT
run git push --set-upstream github gh-pages --force
run git switch -
