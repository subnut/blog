/* Extension for blog source files */
#define SOURCE_EXT ".blog"

/* Directories to convert from and to */
#define SOURCE_DIR "raw"
#define DEST_DIR   "docs"

/* Configuration for htmlize() */
#define READAHEAD 30
#define HISTORY   5

/*
 * Needed for htmlize()
 * NOTE: The effective values are actually one less than what is defined here.
 *       Please adjust the values accordingly.
 */
#define MAX_LINKS       50
#define MAX_LINE_LENGTH 500

/* Used in index.c */
#define MAX_FILES        100
#define MAX_TITLE_LENGTH 150

#define FAVICON "<link rel=icon href=\"data:image/png;base64,\n\
iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAACXBIWXMAAAnXAAAJ1wGxbhe3AAAA\n\
GXRFWHRTb2Z0d2FyZQB3d3cuaW5rc2NhcGUub3Jnm+48GgAAAchJREFUWIXF1j1oFFEUxfHfrF+J\n\
hVpFAqksxEoU0TQhRoiNplOs7ATtLERBLPRFMIWCvZ1NCjsbiWAQFUQFwU4iqIUIFgZJIiJmXdci\n\
6Ca7bzOj+2ZzujnvMuc/9743M6yxssKV406pO4YNCXJnbHTVJZ/XFyoPTqu7lSD4j0YtOoDBYh0I\n\
XmFPQoAlrbMjHyAYwAf/Mq5i+oG+SoHCsRLCazgvWCiyB45GvEnc7gBgRvCRvCe7qdeCWWxuWjkk\n\
eNQBwF+tPoKvRiPh8/o9TRGeD1A3FnGnnFHtBkCGIxH/Xqrw1QGCvRhocmu43x0A0fY/E8ymBGh/\n\
CoLnGEyYNScz4Yob+QBBHz7J26T/o4oRlz1uXMa1s5Rw+GX/Sp64XuNbKQCZN/kAwRccx7vE8VU9\n\
K9+gqT8yDY27oO56k/tEcHC5Uc6coe5wxH3QbJQDEPRgqMWvdAuAYfQ2eXN2edktgFj7p51QW0uA\n\
lvaXA3DNduyOrEzHypf/kmWCfdjWEUDVkNbj/Vbwvj3A0q6dwkhH4e0VbT+NEZwsMZwsDyDTX1o4\n\
P23yMA/gLhZLArjjovl2i43NEgzjLLYmCq7jhS0mnPM90T3T6zemFFVQZGM2gAAAAABJRU5ErkJg\n\
gg==\">"

#define FOOTER  "<footer>\n\
                <hr>\n\
                Unless specified otherwise, text on this website is licensed under\n\
                <a href=\"https://creativecommons.org/licenses/by-sa/4.0/\">CC&nbsp;BY-SA&nbsp;4.0</a>\n\
                and code on this website is licensed under <a href=\"LICENSE.txt\">MIT</a>\n\
            </footer>"

// vim:et:ts=4:sts=0:sw=0:fdm=syntax:nowrap
