#ifndef LINES_H
#define LINES_H

typedef struct Line {
    size_t      cap;
    size_t      len;
    char        *buf;
    int         selected;
    struct Line *prev;
    struct Line *next;
} Line;

typedef struct {
    Line *head;
    Line *tail;
} LineList;

LineList *linelist_create(void);
void     linelist_free(LineList *list);

void     linelist_append(LineList *list, const char *text);
void     linelist_remove(LineList *list, Line *node);
Line     *linelist_insert_after(LineList *list, Line *after, const char *text);

void     linelist_foreach(LineList *list, void (*cb)(Line *, void *), void *ctx);
void     linelist_cb_print(Line *line, void *ctx);
void     linelist_cb_print_selected(Line *line, void *ctx);

#endif
