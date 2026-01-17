#ifndef CONFIG_H
#define CONFIG_H

/*
 * Colors (from termbox)
 * TB_BLACK, TB_RED, TB_GREEN, TB_YELLOW
 * TB_BLUE, TB_MAGENTA, TB_CYAN, TB_WHITE
 */
#define ACCENT_COLOR     TB_CYAN

#define TAB_WIDTH     4

#define SHELL_COMMAND "sh"

#define HELP_TEXT     "Ctrl+Q: exit | Ctrl+S: exec & exit | Esc/Ctrl+3: menu"
static const char *g_menu[] = {
    ".--------------------------------------.",
    "|         Interactive Commands         |",
    "|                Editor                |",
    "|--------------------------------------|",
    "|  esc/q  close this menu              |",
    "|  y      exit & execute               |",
    "|  n      exit                         |",
    "'--------------------------------------'",
};

/* ctrl+c ctrl+c always works,
 * this line is an additional exit key */
#define KEY_EXIT TB_KEY_CTRL_Q

#define KEY_EXIT_EXECUTE TB_KEY_CTRL_S

/* only esc clash with ctrl+3
 * */
#define KEY_MENU_TOGGLE TB_KEY_ESC

#endif
