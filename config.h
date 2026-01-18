#ifndef CONFIG_H
#define CONFIG_H

/*
 * Colors (from termbox)
 * TB_BLACK, TB_RED, TB_GREEN, TB_YELLOW
 * TB_BLUE, TB_MAGENTA, TB_CYAN, TB_WHITE
 */
#define ACCENT_COLOR TB_CYAN

#define TAB_WIDTH 4

#define SHELL_COMMAND "sh"

#define HELP_TEXT "Ctrl+Q: exit, Ctrl+S: exit & exec"

static const char *g_usage =
"ice - interactive commands editor\n"
"\n"
"usage: ice [-h] [-e] [-c]\n"
"\n"
"flags:\n"
"   -h  show this help and exit\n"
"   -e  show exit code after execution\n"
"   -c  print commands before execution\n"
"\n"
"description:\n"
"   ice is a TUI editor for interactive command composition.\n"
"   edit commands in a familiar editor interface, then execute\n"
"   them as a bash script.\n"
"\n"
"   also you can edit config.h to change some default settings.\n"
"\n"
"global controls:\n"
"   ctrl+c / ctrl+q          exit without execution\n"
"   ctrl+s                   exit and execute commands\n"
"\n"
"edit mode controls:\n"
"   arrow keys               navigate\n"
"   ctrl + left/right        jump by word\n"
"   ctrl+w / ctrl+backspace  delete left word\n"
"   tab                      insert 4 spaces\n"
"   enter                    insert new line\n"
"   backspace                delete left symbol\n"
"   any printable ascii      insert character\n"
;

/* ctrl+c always works,
 * this line is an additional exit key */
#define KEY_EXIT TB_KEY_CTRL_Q

#define KEY_EXIT_EXECUTE TB_KEY_CTRL_S

#endif
