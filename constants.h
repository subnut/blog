#define SOURCE_EXT ".blog"
#define SOURCE_DIR "src"
#define DEST_DIR   "docs"
#define MAX_LINE_LENGTH 500
#define MAX_FILES       100
#define MAX_LINKS       100

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
            and code on this website is licensed under\n\
            <a href=\"https://opensource.org/licenses/MIT\">MIT</a>\n\
        </footer>"


/*
 * HTML Named Character References
 *
 * The first number indicates the number of elements in this array
 * The second number indicates (the max number of letters in any element) + 1       // +1 for '\0'
 */
const char named_references[2][7] =
{
    "&nbsp;",
    "&copy;",
};

// vim:et:ts=4:sts=0:sw=0:fdm=syntax:nowrap
