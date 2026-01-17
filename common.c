#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "common.h"

void
die(const char *errstr, ...)
{
    va_list ap;

    va_start(ap, errstr);
    vfprintf(stderr, errstr, ap);
    va_end(ap);
    exit(1);
}
