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
Line     *linelist_insert_after(LineList *list,
                                Line *after,
                                const char *text);
void     linelist_print(LineList *list, FILE *output);

#endif
