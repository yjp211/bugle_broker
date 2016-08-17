#include <pthread.h>
#include <sys/time.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_struct.h>
#include <evws/evws.h>
#include "include.h"
#include "conn.h"
#include "mqtt.h"
#include "map.h"
#include "misc.h"
#include "topic.h"

static int conn_unsubscribe_all(struct conn_context *conn);

map_t g_valid_conn_map;
      
int init_valid_conn_map(int size, int step)
{
    return map_init(&g_valid_conn_map, size, step);
}


int valid_conn_add_member(struct conn_context *conn)
{
    if(NULL == conn){
        return ERR_WILD_POINTER;
    }

    if(!is_conn_active(conn)){
        return ERR_CONN_BREAK;
    }

    int rc = map_set(&g_valid_conn_map, conn->client_id, conn, 0);
    if(rc == 0){
        return SUCCESS;
    }else if(rc == 1){
        return ERR_CLIENT_CONFLICT;
    }else {
        return ERR_SYS_FAILED;
    }
}

int valid_conn_remove_member(struct conn_context *conn)
{

    if(NULL == conn){
        return SUCCESS;
    }
    
    struct conn_context *tmp = NULL;

    tmp = (struct conn_context *)map_remove(&g_valid_conn_map, conn->client_id);
    if(tmp == NULL || conn != tmp){
        return ERR_CLIENT_CONFLICT;
    }
    return SUCCESS;
}

struct conn_context *find_conn_by_clientid(const char *client_id)
{

    if(NULL == client_id || strlen(client_id) == 0) {
        return NULL;
    }
    
    return (struct conn_context *)map_get(&g_valid_conn_map, client_id);
}


int set_conn_unactive(struct conn_context *conn)
{
    if(NULL != conn && conn->is_active){
        //取消订阅所有的topic
        conn_unsubscribe_all(conn);
        
        if(conn->conn_type == CONN_TYPE_TCP){
            if(NULL != conn->bev){
                conn->bev->cbarg = NULL;
                bufferevent_disable(conn->bev, EV_READ|EV_WRITE);
                bufferevent_free(conn->bev);
                conn->bev = NULL;
            }

            if(-1 != conn->socket_fd){
                //close(conn->socket_fd);
                conn->socket_fd = -1;
            }
        }else {
            if(NULL != conn->wsctx){
                evwsconn_free_bev(conn->wsctx);
            }
        
        }

        conn->is_active = 0;
    }
    return SUCCESS;
}

int is_conn_active(struct conn_context *conn)
{
    if(NULL != conn){
        return conn->is_active;
    }
    return 0;
}

int set_conn_authed(struct conn_context *conn)
{
    if(NULL != conn){
        conn->is_auth = 1;
    }
    return SUCCESS;
}

int set_conn_unauth(struct conn_context *conn)
{
    if(NULL != conn){
        conn->is_auth = 0;
    }
    return SUCCESS;
}

int is_conn_auth(struct conn_context *conn)
{
    if(NULL != conn){
        return conn->is_auth;
    }
    return 0;
}


int conn_refer_inc(struct conn_context *conn)
{
    atomic_inc(&conn->refer_count);
    return 0;
}

int conn_refer_dec(struct conn_context *conn)
{
    atomic_dec(&conn->refer_count);
    return 0;
}

int get_conn_refer(struct conn_context *conn)
{
    if(NULL != conn){
        return conn->refer_count;
    }
    return 0;
}

int conn_have_refer(struct conn_context *conn)
{
    if(NULL != conn){
        if(conn->refer_count == REFER_INIT_COUNT){
            return 0;
        }else {
            return 1;
        }
    }
    return 0;
}

int get_cur_time_serie(char *time_serie)
{
    struct timeval now;

    if(gettimeofday(&now, NULL) == 0){
        snprintf(time_serie, TIME_SERIE_LEN, "%ld.%d",
            now.tv_sec, now.tv_usec);
        return SUCCESS;
    }

    return ERR_SYS_FAILED;

}

int conn_add_topic(struct conn_context *conn, const char *topic)
{
    if(NULL == conn){
        return ERR_WILD_POINTER;
    }

    if(NULL == conn->sub_topics){
        conn->sub_topics = strdup(topic);
        return SUCCESS;
    }

    int rc = SUCCESS;
    char *new_str = NULL;

    if((rc = str_add_tok(&new_str, conn->sub_topics, topic, ',')) != SUCCESS){
        return rc;
    }


    free(conn->sub_topics);
    conn->sub_topics = new_str;

    return rc;
}

int conn_remove_topic(struct conn_context *conn, const char *topic)
{
    if(NULL == conn || NULL == conn->sub_topics){
        return SUCCESS;
    }

    if(strlen(topic) == strlen(conn->sub_topics)){
        free(conn->sub_topics);
        conn->sub_topics = NULL;
        return SUCCESS;
    }

    int rc = SUCCESS;
    char *new_str = NULL;
    
    if((rc = str_del_tok(&new_str, conn->sub_topics, topic, ',')) != SUCCESS){
        return rc;
    }


    free(conn->sub_topics);
    conn->sub_topics = new_str;

    return rc;
}

static int conn_unsubscribe_all(struct conn_context *conn)
{
    if(NULL == conn || NULL == conn->sub_topics){
        return SUCCESS;
    }
	char	*buf, delimstr[2];
	char	*tmp, *ptr;
	
    if ((buf = strdup(conn->sub_topics)) == NULL) {
		ERROR("strdup %d failed: %m\n", (int)sizeof(conn->sub_topics));
		return ERR_NOMEM;
	}
	tmp = buf;
	delimstr[0] = ',';
	delimstr[1] = '\0';
	while ((ptr = strsep(&tmp, delimstr)) != NULL) {
		if (strlen(ptr) > 0) {
            topic_remove_client(ptr, conn->client_id);
		}
	}
	free( buf);
    return SUCCESS;

}

int init_connect_context(struct conn_context **conn)
{
    int i = 0;
    struct conn_context *tmp;

    if((tmp = (struct conn_context *)malloc(sizeof(struct conn_context))) == NULL){
        ERROR("malloc %d space failed, %m", (int)sizeof(struct conn_context));
        return ERR_NOMEM;
    }

    tmp->conn_type = CONN_TYPE_TCP;
    tmp->socket_fd = -1;
    tmp->buf =  evbuffer_new();
    tmp->packet = NULL;
    tmp->client_id = NULL;
    tmp->user_name = NULL;
    tmp->passwd = NULL;
    tmp->address = NULL;
    tmp->is_auth = 0;
    tmp->can_pub = 0;
    
    tmp->bev = NULL;
    tmp->is_active = 0;

    tmp->refer_count = REFER_INIT_COUNT;
    bzero(tmp->time_serie, 0);
    get_cur_time_serie(tmp->time_serie);
    
    pthread_mutex_init(&(tmp->write_lock), NULL);
    

    tmp->wsctx = NULL;

    tmp->sub_topics = NULL;

    *conn = tmp;
    return SUCCESS;
}

int destroy_connect_context(struct conn_context *conn)
{
    if(!conn){
        return SUCCESS;
    }

    if(conn->conn_type == CONN_TYPE_TCP){
    
        if(NULL != conn->bev){
            conn->bev->cbarg = NULL;
            bufferevent_disable(conn->bev, EV_READ|EV_WRITE);
            bufferevent_free(conn->bev);
            conn->bev = NULL;
        }

        if(-1 != conn->socket_fd){
            //close(conn->socket_fd);
            conn->socket_fd = -1;
        }
    }else{
        if(NULL != conn->wsctx){
            evwsconn_free(conn->wsctx);
            conn->wsctx = NULL;
        }
    }

    if(NULL != conn->buf){
        //if(evbuffer_get_length(conn->buf) > 0){
            evbuffer_free(conn->buf);
        //}
        conn->buf = NULL;
    }

    if(NULL != conn->packet){
        destroy_mqtt_packet(conn->packet);
        conn->packet = NULL;
    }

    if(conn->client_id != NULL){
        free(conn->client_id);
        conn->client_id = NULL;
    }
    if(conn->user_name != NULL){
        free(conn->user_name);
        conn->user_name=  NULL;
    }
    if(conn->passwd != NULL){
        free(conn->passwd);
        conn->passwd = NULL;
    }
    if(conn->address != NULL){
        free(conn->address);
        conn->address = NULL;
    }


    pthread_mutex_destroy(&(conn->write_lock));

    if(conn->sub_topics!= NULL){
        free(conn->sub_topics);
        conn->sub_topics =  NULL;
    }

    if(NULL != conn) {
        free(conn);
        conn = NULL;

    }

    return SUCCESS;
}



