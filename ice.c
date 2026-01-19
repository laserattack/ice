#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *argv0;

#define TB_IMPL
#include "thirdparty/termbox2.h"

#include "thirdparty/arg.h"

#include "config.h"
#include "common.h"
#include "lines.h"

typedef struct {
    LineList *lines;          /* lines list               */
    Line     *cl;             /* current line             */
    size_t   cp;              /* current position in line */
    int      execute_on_exit; /* 1 or 0 */
} State;

static State g_state = {};

static void
state_init()
{
    g_state.lines           = linelist_create();
    linelist_append(g_state.lines, "");
    g_state.cl              = g_state.lines->head;
    g_state.cp              = 0;
    g_state.execute_on_exit = 0;
}

static void
state_cleanup()
{
    linelist_free(g_state.lines);
}

static void
draw_screen()
{
    /* terminal size */
    size_t th     = tb_height(), tw = tb_width();
    Line   *l     = g_state.lines->head;
    size_t vshift = 0, hshift = 0;
    size_t x      = 0, y = 0;
    size_t line   = 0;

    /* clear screen */
    tb_clear();

    /* calculate vertical shift for scrolling */
    for (;l != g_state.cl;line++,l=l->next);
    if (line > th - 2)
        vshift = line - th + 2;

    /* calculate horizontal shift for scrolling */
    if (g_state.cp > tw - 1)
        hshift = g_state.cp - tw + 1;

    l = g_state.lines->head;

    for (;l;l=l->next,y++) {
        uintattr_t fg = TB_DEFAULT, bg = TB_DEFAULT;

        if (y < vshift) continue;

        if (l == g_state.cl) {
            /* current line */
            for (x = hshift; x < l->len; x++) {

                /* selection */
                if (l->selected) {
                    fg = TB_BLACK;
                    bg = TB_WHITE;
                }

                if (x == g_state.cp) {
                    fg = TB_BLACK;
                    bg = ACCENT_COLOR;
                }

                tb_set_cell(x-hshift, y-vshift, l->buf[x], fg, bg);

                fg = TB_DEFAULT;
                bg = TB_DEFAULT;
            }

            if (g_state.cp == l->len)
                tb_set_cell(l->len-hshift, y-vshift, ' ',
                        TB_BLACK, ACCENT_COLOR);
        } else {
            /* other lines */
            if (l->selected) {
                fg = TB_BLACK;
                bg = TB_WHITE;
            }
            for (x = hshift; x < l->len; x++) {
                tb_set_cell(x-hshift, y-vshift, l->buf[x], fg, bg);
            }
        }
    }

    /* print msgline */
    for (x = 0; x < tw; ++x)
        tb_set_cell(x, th-1, ' ', TB_DEFAULT, TB_DEFAULT);
    tb_printf(0, th-1, ACCENT_COLOR | TB_BOLD, TB_DEFAULT, HELP_TEXT);

    /* draw screen */
    tb_present();
}

static int
valid_char(int ch)
{
    /* printable ascii symbols */
    return ch >= 32 && ch <= 126;
}

static int
handle_events()
{
    struct tb_event ev;
    tb_poll_event(&ev);

    /* edit mode events */
    switch (ev.type) {
    case TB_EVENT_KEY:
        switch (ev.key) {
        /* exit */
        case TB_KEY_CTRL_C: /* fallthrough */
        case KEY_EXIT:
            return 1;

        case KEY_EXIT_EXECUTE:
            g_state.execute_on_exit = 1;
            return 1;

        case KEY_SELECT_LINE:
            g_state.cl->selected = (g_state.cl->selected+1)%2;
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
                Line *cur          = g_state.cl;
                char *after_cursor = &cur->buf[g_state.cp];
                Line *newline      = linelist_insert_after(
                        g_state.lines,
                        cur,
                        after_cursor);

                /* move selection logic */
                if (cur->selected) {
                    if (g_state.cp != cur->len)
                        cur->selected = 0;
                    if (!g_state.cp && cur->len)
                        newline->selected = 1;
                }

                cur->buf[g_state.cp] = 0;
                cur->len             = g_state.cp;
                g_state.cl           = newline;
                g_state.cp           = 0;

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

static void
tui_loop()
{
    /* init termbox */
    tb_init();

    draw_screen();
    /* main loop */
    while (1) {
        if (handle_events()) break;
        draw_screen();
    }

    /* cleanup */
    tb_shutdown();
}

static int
execute_commands(int flag_no_commands_output)
{
    FILE *sh;

    sh = flag_no_commands_output
        ? popen(SHELL_COMMAND" >/dev/null 2>&1", "w")
        : popen(SHELL_COMMAND, "w");

    if (!sh)
        die("open shell error\n");

    linelist_print(g_state.lines, sh);
    return pclose(sh);
}

int
main(int argc, char *argv[])
{
    int exitcode = 0;

    int flag_show_exitcode      = 0;
    int flag_print_commands     = 0;
    int flag_no_commands_output = 0;

    ARGBEGIN {
        case 'h':
            die(g_usage);
            break;
        case 'e':
            flag_show_exitcode = 1;
            break;
        case 'c':
            flag_print_commands = 1;
            break;
        case 'n':
            flag_no_commands_output = 1;
            break;
        default:
            printf(g_usage);
            die("\nunknown flag '%c'\n", ARGC());
    } ARGEND;

    state_init();

    tui_loop();

    if (flag_print_commands) {
        linelist_print(g_state.lines, stdout);
    }

    if (g_state.execute_on_exit)
        exitcode = execute_commands(flag_no_commands_output);

    if (flag_show_exitcode)
        printf("%d\n", exitcode);

    state_cleanup();
    return 0;
}
