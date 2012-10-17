
#ifndef __QWC_STATE__
#define __QWC_STATE__
enum qwc_state
{
    qwc_state_idle,
    qwc_state_connecting,
    qwc_state_connected
};
#endif



struct qw_connection *QWC_Create(char *packet_dump_filename);
void QWC_Connect(struct qw_connection *qwc, char *host, char *userinfo, unsigned int port);
void QWC_Disconnect(struct qw_connection *qwc);
void QWC_Say(struct qw_connection *qwc, char *text);
void QWC_SetSaveConsoleOutput(struct qw_connection *qwc, int save);
//returns one at a time, whoever calls it needs to free it after!
char *QWC_GetConsoleOutput(struct qw_connection *qwc);
void QWC_Set_FPS(struct qw_connection *qwc, unsigned int fps);
void QWC_Quit(struct qw_connection *qwc);
int QWC_GetQuit(struct qw_connection *qwc);
enum qwc_state QWC_GetState(struct qw_connection *qwc);
