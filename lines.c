#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "common.h"
#include "lines.h"

static Line *
line_create(const char *text)
{
    Line *node;

    if (!(node = malloc(sizeof(Line))))
        die("line alloc err\n");

    node->selected = 0;
    node->len      = text? strlen(text): 0;
    node->cap      = node->len + 16;
    if (!(node->buf = malloc(node->cap)))
        die("line buf alloc err\n");

    if (text)
        strcpy(node->buf, text);
    else
        node->buf[0] = 0;


    node->prev = NULL;
    node->next = NULL;

    return node;
}

static void
line_free(Line *node)
{
    if (!node) return;
    free(node->buf);
    free(node);
}

LineList *
linelist_create(void)
{
    LineList *list = malloc(sizeof(LineList));
    if (!list)
        die("linelist alloc err\n");

    list->head = list->tail = NULL;
    return list;
}

void
linelist_free(LineList *list)
{
    Line *node;
    if (!list) return;

    node = list->head;
    while (node) {
        Line *next = node->next;
        line_free(node);
        node = next;
    }

    free(list);
}

void
linelist_append(LineList *list, const char *text)
{
    Line *node = line_create(text);

    if (!list->head) {
        list->head = list->tail = node;
    } else {
        list->tail->next = node;
        node->prev       = list->tail;
        list->tail       = node;
    }
}

void
linelist_remove(LineList *list, Line *node)
{
    if (!list || !node) return;

    if (node->prev)
        node->prev->next = node->next;
    else
        list->head = node->next;

    if (node->next)
        node->next->prev = node->prev;
    else
        list->tail = node->prev;

    line_free(node);
}

Line *
linelist_insert_after(
        LineList   *list,
        Line       *after,
        const char *text)
{
    Line *newline;
    if (!list || !after) return NULL;

    newline       = line_create(text);
    newline->prev = after;
    newline->next = after->next;

    if (after->next)
        after->next->prev = newline;
    else
        list->tail = newline;

    after->next = newline;

    return newline;
}

// start: travers funcs

void
linelist_foreach(
        LineList *list,
        void     (*cb)(Line *, void *),
        void     *ctx)
{
    Line *cur;

    if (!list) return;

    cur = list->head;
    while (cur) {
        cb(cur, ctx);
        cur = cur->next;
    }
}

void
linelist_cb_print(Line *line, void *ctx)
{
    FILE *output = (FILE *)ctx;
    fprintf(output, "%s\n", line->buf);
}

void
linelist_cb_print_selected(Line *line, void *ctx)
{
    if (line->selected) linelist_cb_print(line, ctx);
}

// end: traverse funcs
