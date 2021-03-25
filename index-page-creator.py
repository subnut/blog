"""
Simply recurses through the files in the docs directory, and makes an
index html that indexes all of them, by date (newest first; i.e. simply
use the filename - start from the biggest number and end at 1, NOT 0!)

By 'indexes all of them', I mean it creates a list of links. The link
text is the Title of that post, and the link points to, _obviously_, the
blog post.
"""
