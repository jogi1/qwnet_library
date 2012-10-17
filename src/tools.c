#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include "quakedef.h"


void Tools_Init(struct qw_connection *qwc)
{

}

void QWC_DPInit(struct qw_connection *qwc)
{
    FILE *f;

    if (qwc->packet_dump_filename == NULL)
        return;

    f = fopen(qwc->packet_dump_filename, "w");
    if (f == NULL)
        return;

    fprintf(f, "packet %i:\n", qwc->packet_count);

    fclose(f);
}

void QWC_DPPrintf(struct qw_connection *qwc, const char *fmt, ...)
{
    va_list argptr;
    int i;
    FILE *f;

    if (qwc->packet_dump_filename == NULL)
        return;

    f = fopen("packet_dump", "a");
    if (f == NULL)
        return;

    va_start(argptr, fmt);
    i = vfprintf(f, fmt, argptr);
    va_end (argptr);

    fclose(f);
}
