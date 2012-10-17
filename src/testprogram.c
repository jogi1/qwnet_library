#include <stdlib.h>
#include <string.h>
#include "qwc_functions.h"


void main (void)
{
    struct qw_connection *qwc;

    int i;
    char *c;

    qwc = QWC_Create(NULL);

    if (qwc == NULL)
    {
        printf("qwc_creation failed!\n");
        return;
    }

    QWC_SetSaveConsoleOutput(qwc, 1);
    QWC_Connect(qwc, "qw.dybbuk.de:27700", "\\name\\test\\", 22000);
    QWC_Set_FPS(qwc, 72);

    while (QWC_GetQuit(qwc) == 0 && QWC_GetState(qwc) > qwc_state_idle)
    {
        while ((c = QWC_GetConsoleOutput(qwc)) != NULL)
        {
            printf("%s", c);
            if (strstr(c, "test insult"))
                QWC_Say(qwc,"you suck korni!");
            if (strstr(c, "test quit"))
            {
                QWC_Quit(qwc);
                printf("quitting!\n");
            }
            free(c);
        }
        sleep(1);
        printf("alive!\n");
    }
    free(qwc);
}
