#ifndef __QW_CONNECTION__
#define __QW_CONNECTION__
#define MAX_MSGLEN 1450

#define QWC_IDLE 1
#define QWC_CONNECTING 2

#ifndef __QWC_STATE__
#define __QWC_STATE__
enum qwc_state
{
    qwc_state_idle,
    qwc_state_connecting,
    qwc_state_connected
};
#endif

struct qwc_string
{
    char *string;
    qboolean done;

    struct qwc_string *next, *prev;
};

struct qw_connection
{
    char buffer[2048];
    int buffer_position;
    struct NetQW *netqw;
    struct NetData *netdata;
    double time, oldtime, newtime;

    struct netaddr net_from;
    struct netchan_s netchan;

    struct sizebuf_s net_message;
    byte net_message_buffer[MAX_MSGLEN * 2];
    unsigned int net_message_size;

    unsigned int startframe, numframes;

    struct frame_s frames[UPDATE_BACKUP];

    struct sizebuf_s *msg_message;
    qboolean msg_badread;

    unsigned int msg_readcount;

    int cl_parsecount;
    int oldparsecount, cl_oldparsecount, oldparsecountmod;
    int parsecountmod;

    qboolean quit;
    qboolean run_thread;
    qboolean abort;

    qboolean save_console_output;
    struct qwc_string *console_output;

    double parsecounttime;

    double realtime;
    double extratime;

    char readstring_buffer[2048];

    char *packet_dump_filename;
    unsigned int packet_count;

    FILE *download_file;
    int download_file_percentage;

    int disconnecting;
    unsigned int sleep_granularity;

    unsigned int fps;
    enum qwc_state state;

    struct SysThread *thread;


    // ---------------
    int servercount;
    int protoversion;
    int validsequence, oldvalidsequence;

};

#endif
