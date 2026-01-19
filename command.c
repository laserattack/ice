#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "common.h"
#include "command.h"

static Cmd *
line_create(const char *text)
{
    Cmd *node;

    if (!(node = malloc(sizeof(Cmd))))
        die("line alloc err\n");

    node->len = text? strlen(text): 0;
    node->cap = node->len + 16;
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
line_free(Cmd *node)
{
    if (!node) return;
    free(node->buf);
    free(node);
}

CmdList *
cmdlist_create(void)
{
    CmdList *list = malloc(sizeof(CmdList));
    if (!list)
        die("cmdlist alloc err\n");

    list->head = list->tail = NULL;
    return list;
}

void
cmdlist_free(CmdList *list)
{
    Cmd *node;
    if (!list) return;

    node = list->head;
    while (node) {
        Cmd *next = node->next;
        line_free(node);
        node = next;
    }

    free(list);
}

void
cmdlist_append(CmdList *list, const char *text)
{
    Cmd *node = line_create(text);

    if (!list->head) {
        list->head = list->tail = node;
    } else {
        list->tail->next = node;
        node->prev       = list->tail;
        list->tail       = node;
    }
}

void
cmdlist_remove(CmdList *list, Cmd *node)
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

Cmd *
cmdlist_insert_after(
        CmdList    *list,
        Cmd        *after,
        const char *text)
{
    Cmd *newline;
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

static void
cmdlist_traverse(
        CmdList *list,
        void    (*cb)(Cmd *, void *),
        void    *ctx)
{
    Cmd *cur;

    if (!list) return;

    cur = list->head;
    while (cur) {
        cb(cur, ctx);
        cur = cur->next;
    }
}

static void
cmdlist_cb_print(Cmd *line, void *ctx)
{
    FILE *output = (FILE *)ctx;
    fprintf(output, "%s\n", line->buf);
}

// end: traverse funcs

void
cmdlist_print(CmdList *list, FILE *output)
{
    cmdlist_traverse(list, cmdlist_cb_print, output);
}
