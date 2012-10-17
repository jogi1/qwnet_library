#include "quakedef.h"


void QWC_StringAdd(struct qw_connection *qwc, char *string)
{
    struct qwc_string *s, *l;

    if (qwc == NULL)
        return;

    if (qwc->save_console_output == false)
        return;

    s= calloc(1, sizeof(*s));

    if (s== NULL)
        return;

    s->string = strdup(string);

    if (s->string == NULL)
    {
        free(s);
        return;
    }

    if (qwc->console_output == NULL)
    {
        qwc->console_output = s;
    }
    else
    {
        l = qwc->console_output;
        while (l->next)
            l = l->next;
        l->next = s;
        s->prev = l;
    }
}
