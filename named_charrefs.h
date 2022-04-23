#ifndef NAMED_CHARREFS_H
#define NAMED_CHARREFS_H

/*
 * Named Character References that will be recognized by is_charref()
 * Visit https://dev.w3.org/html5/html-author/charref for more.
 *
 * Say a named Character Reference is "&abcd;", but hasn't been included in
 * this array. htmlize() will turn it to "&nbsp;abcd;". So, the browser shall
 * show "&abcd;" literally instead of the character that was originally
 * referenced by &abcd;
 */

static const char *named_charrefs[] = {
	"amp"  ,
	"nbsp" ,
	"reg"  , "REG"  ,
	"copy" , "COPY" ,
	"mdash", "ndash",
};

#endif
