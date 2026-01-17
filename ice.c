#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TB_IMPL
#include "termbox2.h"

#include "config.h"
#include "utils.h"

typedef struct Line {
    size_t      cap;
    size_t      len;
    char        *buf;
    struct Line *prev;
    struct Line *next;
} Line;

typedef struct {
    Line *head;
    Line *tail;
} LineList;

typedef enum {
    MODE_EDIT = 1,
    MODE_MENU,
} Mode;

typedef struct {
    Mode     m;      /* current app mode         */
    LineList *lines; /* lines list               */
    Line     *cl;    /* current line             */
    size_t   cp;     /* current position in line */
} State;

typedef enum {
    ERR_GOOD = -228,
} Error;

static State g_state = {};
static Error g_err   = 0;

static Line *
line_create(const char *text)
{
    Line *node;

    if (!(node = malloc(sizeof(Line))))
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
line_free(Line *node)
{
    if (!node) return;
    free(node->buf);
    free(node);
}

static LineList *
linelist_create(void)
{
    LineList *list = malloc(sizeof(LineList));
    if (!list)
        die("linelist alloc err\n");

    list->head = list->tail = NULL;
    return list;
}

static void
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

static void
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

static void
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

static Line *
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

static void
linelist_traverse(
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

static void
linelist_cb_print(Line *line, void *ctx)
{
    FILE *output = (FILE *)ctx;
    fprintf(output, "%s\n", line->buf);
}

// end: traverse funcs

static void
linelist_print(LineList *list, FILE *output)
{
    linelist_traverse(list, linelist_cb_print, output);
}

static void
draw_screen()
{
    /* terminal size */
    size_t tw = tb_width();
    size_t th = tb_height();
    size_t x  = 0, y = 0;

    /* clear screen */
    tb_clear();

    /* prepare screen output */
    switch (g_state.m) {
    case MODE_MENU:
        {
            size_t i, lc;
            char *menu[] = {
                ".-------------------------------------.",
                "|         Interactive Commands        |",
                "|               Editor                |",
                "|-------------------------------------|",
                "|  e  execute commands                |",
                "|  y  execute commands end exit       |",
                "|  n  exit without execution          |",
                "'-------------------------------------'",
            };

            lc = sizeof(menu)/sizeof(*menu);
            for (i=0; i<lc; ++i) {
                size_t j;
                size_t strl   = strlen(menu[i]);
                size_t starty = (th-lc) / 2 + i;
                size_t startx = tw / 2 - strl / 2;

                for (j=0; j<strl; ++j) {
                    char s = menu[i][j];
                    uintattr_t color = (s == 'I' || s == 'C' || s == 'E')
                        ? ACCENT_COLOR
                        : TB_DEFAULT;

                    tb_set_cell(startx+j, starty,
                            menu[i][j], color, TB_DEFAULT);
                }
            }

            break;
        }
    case MODE_EDIT:
        {
            Line   *l     = g_state.lines->head;
            size_t line   = 0;
            size_t vshift = 0, hshift = 0;

            /* calculate vertical shift for scrolling */
            for (;l != g_state.cl;line++,l=l->next);
            if (line > th - 2)
                vshift = line - th + 2;

            /* calculate horizontal shift for scrolling */
            if (g_state.cp > tw - 1)
                hshift = g_state.cp - tw + 1;

            l = g_state.lines->head;

            for (;l;l=l->next,y++) {

                if (y < vshift) continue;

                if (l == g_state.cl) {
                    for (x = hshift; x < l->len; x++) {
                        uint32_t fg = TB_DEFAULT, bg = TB_DEFAULT;

                        if (x == g_state.cp) {
                            fg = TB_BLACK;
                            bg = ACCENT_COLOR;
                        }

                        tb_set_cell(x-hshift, y-vshift, l->buf[x], fg, bg);
                    }

                    if (g_state.cp == l->len)
                        tb_set_cell(l->len-hshift, y-vshift, ' ',
                                TB_BLACK, ACCENT_COLOR);
                } else {
                    for (x = hshift; x < l->len; x++)
                        tb_set_cell(x-hshift, y-vshift, l->buf[x],
                                TB_DEFAULT, TB_DEFAULT);
                }
            }

            /* case break */
            break;
        }
    }

    /* print msgline */
    for (x = 0; x < tw; ++x)
        tb_set_cell(x, th-1, ' ', TB_DEFAULT, TB_DEFAULT);
    tb_printf(0, th-1, TB_BLACK, ACCENT_COLOR,
            "Esc: menu, Run with flag -h: help");

    /* draw screen */
    tb_present();
}

static int
valid_char(int ch)
{
    return ((ch >= 'A' && ch <= 'Z') ||
            (ch >= 'a' && ch <= 'z') ||
            (ch >= '0' && ch <= '9') ||
            ch == '.' || ch == '-' ||
            ch == '_' || ch == '/' ||
            ch == ' '
           );
}

static int
handle_events()
{
    struct tb_event ev;
    tb_poll_event(&ev);

    /* all modes events */
    switch (ev.type) {
    case TB_EVENT_KEY:
        switch(ev.key) {
        case TB_KEY_CTRL_C:
            return g_err = ERR_GOOD;
        }
    }

    /* menu mode events */
    if (g_state.m == MODE_MENU) {
        switch (ev.type) {
        case TB_EVENT_KEY:
            switch (ev.key) {
            /* toggle menu */
            case TB_KEY_ESC:
                g_state.m = MODE_EDIT;
                break;

            default:
                switch(ev.ch) {
                /* toggle menu */
                case 'q': /* fallthrough */
                case 'Q':
                    g_state.m = MODE_EDIT;
                    break;

                case 'e': /* fallthrough */
                case 'E':
                    g_state.m = MODE_EDIT;
                    break;

                case 'y': /* fallthrough */
                case 'Y':
                    return g_err = ERR_GOOD;

                case 'n': /* fallthrough */
                case 'N':
                    return g_err = ERR_GOOD;
                }
            }
        }

        return 0;
    }

    /* edit mode events */
    if (g_state.m == MODE_EDIT) {
        switch (ev.type) {
        case TB_EVENT_KEY:
            switch (ev.key) {
            /* toggle menu */
            case TB_KEY_ESC:
                g_state.m = MODE_MENU;
                break;

            /* delete left symbol */
            case TB_KEY_BACKSPACE:  /* fallthrough */
            case TB_KEY_BACKSPACE2: /* fallthrough */
            /* delete left word (clash with ctrl+backspace) */
            case TB_KEY_CTRL_W:
                {
                    Line *cur = g_state.cl;

                    if (g_state.cp > 0) {
                        if (ev.key == TB_KEY_CTRL_W) {
                            size_t pos = g_state.cp, delete_len;

                            /* del spaces */
                            while (pos && cur->buf[pos-1] == ' ') pos--;
                            /* del word */
                            while (pos && cur->buf[pos-1] != ' ') pos--;

                            delete_len = g_state.cp - pos;
                            if (delete_len > 0) {
                                memmove(&cur->buf[pos],
                                        &cur->buf[g_state.cp],
                                        cur->len-g_state.cp+1);

                                g_state.cp = pos;
                                cur->len -= delete_len;
                            }
                        } else {
                            memmove(&cur->buf[g_state.cp-1],
                                    &cur->buf[g_state.cp],
                                    cur->len-g_state.cp+1);
                            g_state.cp--;
                            cur->len--;
                        }
                    } else if (cur->prev) {
                        /* merge lines case */
                        Line   *prev  = cur->prev;
                        size_t newlen = prev->len + cur->len;

                        if (newlen+1 >= prev->cap) {
                            prev->cap = (newlen+1)*2;
                            prev->buf = realloc(prev->buf,
                                    prev->cap);
                            if (!prev->buf)
                                die("realloc line buf err\n");
                        }

                        strcat(prev->buf, cur->buf);

                        prev->len  = newlen;
                        g_state.cl = prev;
                        g_state.cp = prev->len-cur->len;

                        linelist_remove(g_state.lines, cur);
                    }

                    break;
                }

            /* insert TAB_WIDTH spaces on tab */
            case TB_KEY_TAB:
                {
                    Line *cur = g_state.cl;

                    if (cur->len + TAB_WIDTH >= cur->cap) {
                        cur->cap = (cur->len + TAB_WIDTH) * 2;
                        cur->buf = realloc(cur->buf, cur->cap);
                        if (!cur->buf)
                            die("realloc line buf err\n");
                    }

                    memmove(&cur->buf[g_state.cp+TAB_WIDTH],
                            &cur->buf[g_state.cp],
                            cur->len-g_state.cp+1);

                    memset(&cur->buf[g_state.cp], ' ', TAB_WIDTH);

                    cur->len   += TAB_WIDTH;
                    g_state.cp += TAB_WIDTH;
                    break;
                }

            /* move line */
            case TB_KEY_ENTER:
                {
                    size_t newpos;
                    Line   *curline, *newline;
                    char   *newline_buf, *after_cursor;

                    curline      = g_state.cl;
                    newpos       = strspn(curline->buf, " ");
                    after_cursor = &curline->buf[g_state.cp];

                    if (asprintf(&newline_buf,
                            "%.*s%s",
                            (int)newpos,
                            curline->buf,
                            after_cursor) == -1)
                        die("line buf alloc err\n");

                    newline = linelist_insert_after(g_state.lines,
                            curline,
                            newline_buf);

                    free(newline_buf);

                    curline->buf[g_state.cp] = 0;
                    curline->len             = g_state.cp;

                    g_state.cl = newline;
                    g_state.cp = newpos;

                    break;
                }

            case TB_KEY_ARROW_LEFT:
                if (g_state.cp > 0) {
                    if (ev.mod == TB_MOD_CTRL) {
                        Line   *cur = g_state.cl;
                        size_t pos  = g_state.cp;
                        /* skip spaces */
                        while (pos && cur->buf[pos-1] == ' ') pos--;
                        /* skip word */
                        while (pos && cur->buf[pos-1] != ' ') pos--;
                        g_state.cp = pos;
                    } else {
                        g_state.cp--;
                    }
                } else if (g_state.cl->prev) {
                    g_state.cl = g_state.cl->prev;
                    g_state.cp = g_state.cl->len;
                }
                break;

            case TB_KEY_ARROW_RIGHT:
                if (g_state.cp < g_state.cl->len) {
                    if (ev.mod == TB_MOD_CTRL) {
                        Line   *cur = g_state.cl;
                        size_t pos  = g_state.cp;
                        /* skip word */
                        while (pos < cur->len && cur->buf[pos] != ' ') pos++;
                        /* skip spaces */
                        while (pos < cur->len && cur->buf[pos] == ' ') pos++;
                        g_state.cp = pos;
                    } else {
                        g_state.cp++;
                    }
                } else if (g_state.cl->next) {
                    g_state.cl = g_state.cl->next;
                    g_state.cp = 0;
                }
                break;

            case TB_KEY_ARROW_UP:
                if (g_state.cl->prev) {
                    g_state.cl = g_state.cl->prev;
                    if (g_state.cp > g_state.cl->len)
                        g_state.cp = g_state.cl->len;
                }
                break;

            case TB_KEY_ARROW_DOWN:
                if (g_state.cl->next) {
                    g_state.cl = g_state.cl->next;
                    if (g_state.cp > g_state.cl->len)
                        g_state.cp = g_state.cl->len;
                }
                break;

            default:
                /* insert symbol */
                if (valid_char(ev.ch)) {
                    Line *line = g_state.cl;

                    if (line->len + 1 == line->cap) {
                        line->cap = (line->len + 1) * 2;
                        line->buf = realloc(line->buf, line->cap);
                        if (!line->buf)
                            die("realloc line buf err\n");
                    }

                    memmove(&line->buf[g_state.cp+1],
                            &line->buf[g_state.cp],
                            line->len-g_state.cp+1);

                    line->buf[g_state.cp] = (char)ev.ch;
                    line->len++;
                    g_state.cp++;
                }
                break;
            }
            break;
        }

        return 0;
    }

    return 0;
}

static void
print_err()
{
    switch(g_err) {
    case ERR_GOOD:
        break;
    }
}

static void
tui_loop()
{
    /* init termbox and app state */
    tb_init();
    g_state.m     = MODE_EDIT;
    g_state.lines = linelist_create();
    linelist_append(g_state.lines, "");
    g_state.cl = g_state.lines->head;
    g_state.cp = 0;

    draw_screen();
    /* main loop */
    while (1) {
        if (handle_events()) break;
        draw_screen();
    }

    /* cleanup */
    tb_shutdown();

    linelist_print(g_state.lines, stdout);

    linelist_free(g_state.lines);
    print_err();
}

int
main()
{
    tui_loop();
    return 0;
}
