#include <pthread.h>
#include "include.h"
#include "map.h"
#include "topic.h"


/**订阅树为嵌套map**/

map_t g_topics_map;

extern struct config_t g_config;

//pthread_rwlock_t g_topic_lock = PTHREAD_RWLOCK_INITIALIZER;

int init_topics_map(int size, int step)
{
    return map_init(&g_topics_map, size, step);
}


int topic_add_client(const char *topic, struct conn_context *conn)
{

    if(NULL == conn){
        return ERR_WILD_POINTER;
    }

    if(!is_conn_active(conn)){
        return ERR_CONN_BREAK;
    }
    
    int rc = SUCCESS;

    /**下列操作要保证原子性， 理应加锁， 但加锁带来的开销和一点的体验损失相比，暂不加锁*/
    map_t *clients_map = (map_t *) map_get(&g_topics_map, topic);
    if(NULL == clients_map){
        if((clients_map = (map_t *)malloc(sizeof(map_t))) == NULL){
                    
            ERROR("malloc %d space failed, %m\n", (int)sizeof(map_t));
            return  ERR_NOMEM;
        }
        map_init(clients_map, g_config.sub_init_count, 
                g_config.sub_inrc_step);

        //拷贝字符串， 因为map是保存指针，直接用topic相当于使用堆栈变量，函数退出后会被释放置为''
        char *topic_dup = strdup(topic);
        if((rc = map_set(&g_topics_map, topic_dup, clients_map, 0)) != 0){
            free(clients_map);
            return  ERR_SYS_FAILED;   
        }
    }


    rc = map_set(clients_map, conn->client_id, conn, 1);

    return rc;  
}

int topic_remove_client(const char *topic, const char *client_id)
{
    map_t *clients_map = (map_t *) map_get(&g_topics_map, topic);
    if(NULL == clients_map){
        return SUCCESS;
    }
    map_remove(clients_map, client_id);
    
    return SUCCESS;
}

int topic_get_client_count(const char *topic, int *count)
{
    map_t *clients_map = (map_t *) map_get(&g_topics_map, topic);
    if(NULL == clients_map){
        *count = 0;
        return SUCCESS;
    }
    *count = map_get_count(clients_map);
    return SUCCESS;
}

map_t *topic_get_clients_map(const char *topic)
{
    return (map_t *) map_get(&g_topics_map, topic);
}
