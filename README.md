# Wtf?

ICE - Interactive Command Editor

# Usage

```
ice - interactive command editor

usage: ice [-h] [-e] [-c]

flags:
    -h  show this help and exit
    -e  show exit code after execution
    -c  print commands before execution

description:
    ice is a TUI editor for interactive command composition.
    edit commands in a familiar editor interface, then execute
    them as a bash script.

    also you can edit config.h to change some default settings.

global controls:
    ctrl+c / ctrl+q          exit without execution
    ctrl+s                   exit and execute commands
    esc / ctrl+3             toggle menu

menu options:
    y/Y                      exit and execute commands
    n/N                      exit without execution
    q/Q / esc / ctrl+3       close menu

edit mode controls:
    arrow keys               navigate
    ctrl + left/right        jump by word
    ctrl+w / ctrl+backspace  delete left word
    tab                      insert 4 spaces
    enter                    insert new line
    backspace                delete left symbol
    any printable ascii      insert character
```
