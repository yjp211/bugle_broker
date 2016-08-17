#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>

#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <evws/wslistener.h>
#include <evws/evws.h>

#include "include.h"
#include "conn.h"
#include "mqtt.h"


extern struct thread_pool *broker_pool;

static const char *subprotocols[] = {
    "mqtt",
    "mqttv3.1",
    "",
}; //实际上不 需要这么多，貌似是 evws 库 的一个 bug

static void read_ws_cb(struct evwsconn *wsctx, enum evws_data_type data_type,
        const unsigned char *data, int len, void *arg)
{

    if(NULL == arg){
        return ;
    }
    
    struct conn_context *conn =  arg;
    
    if(!is_conn_active(conn)){
        return;
    }
    
    if(evbuffer_add(conn->buf, data, len) != 0 ) {
        ERROR("read data form socket error, %m\n");
        return;
    }
    
    read_mqtt_packet(conn, broker_pool);

}


static void error_ws_cb(struct evwsconn* wsctx, void *arg) 
{
    //evwsconn_free(conn);
    if(NULL == arg){
        return ;
    }
    
    struct conn_context *conn =  arg;
    
    set_conn_unactive(conn);
    
    
    struct mqtt_packet *packet = NULL;
    if(malloc_mqtt_packet(&packet) != SUCCESS){
        return;
    }
    packet->command = DISCONNECT;
    deliver_packet_to_pool(conn, packet, broker_pool);
}

static void accept_ws_cb(struct evwsconnlistener *wslistener,
    struct evwsconn *wsctx, struct sockaddr *peeraddr, int socklen,
    void *ctx) 
{

    struct event_base *base = NULL;
    struct evconnlistener *listener = NULL;
    
    struct sockaddr_in *sin;
    char address[INET_ADDRSTRLEN];
    
    struct conn_context *conn;
    
    
    sin = (struct sockaddr_in *)peeraddr;
    bzero(address, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &(sin->sin_addr), address, INET_ADDRSTRLEN);
    
    DEBUG(1, "accept a new client from %s:%d\n", address, sin->sin_port);

    
    listener = evconnlistener_get_evconnlistener(wslistener);
    base = evconnlistener_get_base(listener);
    
    if(init_connect_context(&conn) != SUCCESS){
        ERROR("init connection failed\n");
        return;
    }

    conn->conn_type = CONN_TYPE_WEBSOCKETS;
    conn->is_active = 1;
    conn->wsctx = wsctx;
    conn->bev = NULL; //WAR !!!can't get this value
    conn->socket_fd = -1; //WRANA !! socket can't be get by any function
    conn->address = strdup(address);
  
    evwsconn_set_cbs(wsctx, read_ws_cb, error_ws_cb, error_ws_cb, conn);

}

static void accept_ws_error_cb(struct evwsconnlistener *wslistener, void *ctx) 
{
    struct event_base *base = NULL;
    struct evconnlistener *listener = NULL;
    listener = evconnlistener_get_evconnlistener(wslistener);
    base = evconnlistener_get_base(listener);
    
    int err = EVUTIL_SOCKET_ERROR();
    ERROR("got an error %d (%s) on the listener. "
            "shutting down./n", err, evutil_socket_error_to_string(err));

    fprintf(stderr, "got an error %d (%s) on the listener. "
            "shutting down./n", err, evutil_socket_error_to_string(err));

    event_base_loopexit(base, NULL);
}

int start_listen_ws(struct event_base * base, int port)
{

    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(0);
    addr.sin_port = htons(port);

    struct evwsconnlistener *listener; 


    listener = evwsconnlistener_new_bind(base, accept_ws_cb, NULL,
        LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1, subprotocols,
      (struct sockaddr*)&addr, sizeof(addr));
    
    
    if (!listener) {
        return -1;
    }
  
    evwsconnlistener_set_error_cb(listener, accept_ws_error_cb);

    return 0;

}
