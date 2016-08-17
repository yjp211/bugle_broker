#include <stdlib.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

#include <event2/event.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

#include <curl/curl.h>


#include "include.h"
#include "pool.h"
#include "topic.h"
#include "websockets.h"
#include "rpc_handle.h"
#include "broker_handle.h"


#define DEF_CONFIG_PATH "./config.conf"

char *config_path = DEF_CONFIG_PATH;

struct config_t g_config;

char g_host_flag[1024];
struct thread_pool *broker_pool = NULL; 
struct thread_pool *rpc_pool = NULL; 

static void socket_read_cb(struct bufferevent *bev, 
        struct conn_context *conn, struct thread_pool *pool)
{
    if(!is_conn_active(conn)){
        return;
    }

    if(bufferevent_read_buffer(bev, conn->buf) != 0){
        ERROR("read data form socket error, %m\n");
        return;
    }
    
    read_mqtt_packet(conn, pool);
}

static void broker_read_cb(struct bufferevent *bev, void *arg)
{
    if(NULL != arg){
        struct conn_context *conn =  arg;
        socket_read_cb(bev, conn, broker_pool);
    }
}

static void rpc_read_cb(struct bufferevent *bev, void *arg)
{
    if(NULL != arg){
        struct conn_context *conn =  arg;
        socket_read_cb(bev, conn, rpc_pool);
    }
}


static void socket_error_cb(struct conn_context *conn, struct thread_pool *pool)
{
    struct mqtt_packet *packet = NULL;
    if(malloc_mqtt_packet(&packet) != SUCCESS){
        return;
    }
    packet->command = DISCONNECT;
    deliver_packet_to_pool(conn, packet, pool);
}

static void broker_error_cb(struct bufferevent *bev, short error,  void *arg)
{
    if(NULL != arg){
        struct conn_context *conn =  arg;
        if(NULL != conn){
            set_conn_unactive(conn);
            socket_error_cb(conn, broker_pool);
        }
    }
}

static void rpc_error_cb(struct bufferevent *bev, short error,  void *arg)
{
    if(NULL != arg){
        struct conn_context *conn =  arg;
        if(NULL != conn){
            set_conn_unactive(conn);
            socket_error_cb(conn, rpc_pool);
        }
    }
}

static void accept_conn_cb(struct evconnlistener *listener, evutil_socket_t fd,
                    struct sockaddr *peeraddr, int socklen, void *ctx,
                    bufferevent_data_cb readcb,
                    bufferevent_event_cb errorcb)
{
    struct bufferevent *bev;
    struct event_base *base = NULL;

    struct sockaddr_in *sin;
    char address[INET_ADDRSTRLEN];

    struct conn_context *conn;

    base = evconnlistener_get_base(listener);
    

    sin = (struct sockaddr_in *)peeraddr;
    bzero(address, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &(sin->sin_addr), address, INET_ADDRSTRLEN);

    DEBUG(1, "accept a new client from %s:%d\n", address, sin->sin_port);

    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

    if(init_connect_context(&conn) != SUCCESS){
        ERROR("init connection failed\n");
        return;
    }
    conn->conn_type = CONN_TYPE_TCP;
    conn->bev = bev;
    conn->is_active = 1;
    conn->socket_fd = fd;
    conn->address = strdup(address);

    //设置读写对应的回调函数
    bufferevent_setcb(bev, readcb, NULL, errorcb, conn);

    /*
     * 启用读写事件,其实是调用了event_add将相应读写事件加入事件监听队列poll。
     * 正如文档所说，如果相应事件不置为true，buf
     * ferevent是不会读写数据的
     */
    bufferevent_enable(bev, EV_READ|EV_WRITE);
}

static void accept_broker_conn_cb(struct evconnlistener *listener, evutil_socket_t fd,
                    struct sockaddr *peeraddr, int socklen, void *ctx)
{
    accept_conn_cb(listener, fd, peeraddr, socklen, 
            ctx, broker_read_cb, broker_error_cb);
}

static void accept_rpc_conn_cb(struct evconnlistener *listener, evutil_socket_t fd,
                    struct sockaddr *peeraddr, int socklen, void *ctx)
{
    accept_conn_cb(listener, fd, peeraddr, socklen, 
            ctx, rpc_read_cb, rpc_error_cb);
}

static void accept_error_cb(struct evconnlistener *listener, void *ctx)
{
        struct event_base *base = evconnlistener_get_base(listener);
        int err = EVUTIL_SOCKET_ERROR();
        ERROR("got an error %d (%s) on the listener. "
                "shutting down./n", err, evutil_socket_error_to_string(err));
        fprintf(stderr, "got an error %d (%s) on the listener. "
                "shutting down./n", err, evutil_socket_error_to_string(err));
 
        event_base_loopexit(base, NULL);
}

static int start_listen(struct event_base * base, int port, evconnlistener_cb accept_cb)
{
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(0);
    addr.sin_port = htons(port);

    struct evconnlistener *listener;
    
    listener = evconnlistener_new_bind(base, accept_cb, NULL,
            LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1,
           (struct sockaddr*)&addr, sizeof(addr));
    if (!listener) {
        return -1;
    }
    evconnlistener_set_error_cb(listener, accept_error_cb);
    return 0;
}

static struct option options[] = { 
    { "help",  no_argument, NULL, 'c' },
    { "config-path",  required_argument,  NULL, 'c' },
};

static int parse_options(int argc, char **argv)
{
    int n = 0;
    while (n >= 0) {
        n = getopt_long(argc, argv, "c:", options, NULL);
        if(n < 0) continue;
        switch(n){
        case 'c':
            config_path = strdup(optarg);
            break;

        case '?':
        case 'h':
        default:
            fprintf(stderr, "Useage: broker [c:h] <arg>\n"
                    "   -c: config path, default is <./config.conf>\n"
                    "   -h: help\n");
            exit(1);
        }
        
    }

    return SUCCESS;
}

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);

    parse_options(argc, argv);

    if(parse_config(config_path, &g_config) != 0){
        fprintf(stderr, "read config failed, %m\n");
        return -1;
    }

    if(init_log(g_config.log_path, g_config.log_max_size,
        g_config.debug_level) < 0){
        fprintf(stderr, "init log <%s> failed, exit.\n", g_config.log_path);
        return -1;       
    }
    DEBUG(1, "start %s \n", "=========");

    if(init_thread_pool(g_config.rpc_thread_count,
                rpc_handle, &rpc_pool) != 0){
        fprintf(stderr, "init rpc process pool error, %m\n");
        return -1;
    }
    
    if(init_thread_pool(g_config.broker_thread_count,
                broker_handle, &broker_pool) != 0){
        fprintf(stderr, "init broker process pool error, %m\n");
        return -1;
    }
    
    
    struct event_base *base = NULL;
    if(!(base = event_base_new())){
        fprintf(stderr, "event_base_new error, %m\n");
        return -1;
    }

    if(start_listen(base, g_config.rpc_port, accept_rpc_conn_cb) != 0){
        fprintf(stderr, "couldn't create listener at %d, %m\n", 
                g_config.rpc_port);
        return -1;
    }
    

    DEBUG(10, "start listen on rpc socket:%d\n", g_config.rpc_port);

    int i = 0;
    int port = 0;
    if(!g_config.with_websockets){
        for(i=0; i<g_config.listen_count; i++){
            port = g_config.broker_port + i;
            if(start_listen(base, port, accept_broker_conn_cb) != 0){
                fprintf(stderr, "couldn't create listener at %d, %m\n", port);
                return -1;
            }
            DEBUG(10, "start listen on broker socket:%d\n", port);
        }
    }else {
        for(i=0; i<g_config.listen_count; i++){
            port = g_config.webscoket_port + i;
            if(start_listen_ws(base, port) != 0){
                fprintf(stderr, "couldn't create listener at %d, %m\n", port);
                return -1;
            }
            DEBUG(10, "start listen on broker [web socket]:%d\n", port);
        }
    }
    
    //init libcurl
    curl_global_init(CURL_GLOBAL_NOTHING); 

    init_valid_conn_map(g_config.client_init_count,
             g_config.client_inrc_step);
    init_topics_map(g_config.topic_init_count, 
            g_config.topic_inrc_step);

    //event_base_dispatch(base);
    event_base_loop(base, EVLOOP_NO_EXIT_ON_EMPTY);
}

