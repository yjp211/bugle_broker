#ifndef _CONN_H_
#define _CONN_H_

#include <pthread.h>


#define atomic_inc(x) __sync_add_and_fetch((x),1)  
#define atomic_dec(x) __sync_sub_and_fetch((x),1)  

#define MAX_MSG_ID 65535
#define MAX_UUID_LEN 100
#define IDMAP_INC_STEP  1000

#define TIME_SERIE_LEN 20


#define REFER_INIT_COUNT 0

#define CONN_REFER_INC(conn)  conn_refer_inc(conn)
#define CONN_REFER_DEC(conn)  conn_refer_dec(conn)
#define CONN_HAVE_REFER(conn) conn_have_refer(conn)

enum conn_scoket_type{
    CONN_TYPE_TCP = 0,
    CONN_TYPE_WEBSOCKETS = 1,
};

struct conn_context{
    enum conn_scoket_type conn_type;
    int socket_fd;
    struct evbuffer *buf;
    struct mqtt_packet *packet;
    char *client_id;
    char *user_name;
    char *passwd;
    char *address; //client address
    int is_auth; // 0 means not
    int can_pub; //it's this a publish client

    struct bufferevent *bev;
    int is_active;

    //char **id_map;
    //int map_len;
    //pthread_mutex_t id_map_lock; //access id_map shoud get thread lock
    //


    pthread_mutex_t write_lock; //access id_map shoud get thread lock

    int refer_count; //引用计数

    char time_serie[TIME_SERIE_LEN]; //登录的时间序列，在conn accept时设置


    struct evwsconn *wsctx;


    char *sub_topics;
};



int init_valid_conn_map(int size, int step);
int valid_conn_add_member(struct conn_context *conn);
int valid_conn_remove_member(struct conn_context *conn);
struct conn_context *find_conn_by_clientid(const char *client_id);

#if 0
int gain_msgid_map_pubid(const char *publish_id, struct conn_context *conn, uint16_t *msg_id);
int get_pubid_map_msgid(struct conn_context *conn, uint16_t msg_id, char **publish_id);
int release_msgid(struct conn_context *conn, uint16_t msg_id);
#endif

int set_conn_unactive(struct conn_context *conn);
int is_conn_active(struct conn_context *conn);

int set_conn_authed(struct conn_context *conn);
int set_conn_unauth(struct conn_context *conn);
int is_conn_auth(struct conn_context *conn);


int conn_refer_inc(struct conn_context *conn);
int conn_refer_dec(struct conn_context *conn);
int get_conn_refer(struct conn_context *conn);
int conn_have_refer(struct conn_context *conn);

int get_cur_time_serie();
int init_connect_context(struct conn_context **conn);
int destroy_connect_context(struct conn_context *conn);


int conn_add_topic(struct conn_context *conn, const char *topic);
int conn_remove_topic(struct conn_context *conn, const char *topic);
#endif
