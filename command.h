#ifndef COMMAND_H
#define COMMAND_H

typedef struct Cmd {
    size_t     cap;
    size_t     len;
    char       *buf;
    struct Cmd *prev;
    struct Cmd *next;
} Cmd;

typedef struct {
    Cmd *head;
    Cmd *tail;
} CmdList;

CmdList *cmdlist_create(void);
void    cmdlist_free(CmdList *list);
void    cmdlist_append(CmdList *list, const char *text);
void    cmdlist_remove(CmdList *list, Cmd *node);
Cmd     *cmdlist_insert_after(CmdList *list, Cmd *after, const char *text);
void    cmdlist_print(CmdList *list, FILE *output);

#endif
