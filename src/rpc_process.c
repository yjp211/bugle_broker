#include "include.h"
#include "common_process.h"
#include "rpc_process.h"
#include "map.h"
#include "topic.h"

extern struct config_t g_config;

static int gain_raw_publish_packet(char *topic_name, char *msg, struct mqtt_packet **packet);
static int gain_raw_tonc_ack_packet(int online, struct mqtt_packet **packet);
static int push_msg_to_conn(struct conn_context *conn, struct mqtt_packet *packet);


int rpc_publish_pure_process(char *publish_id, char *topic, char *message)
{
    int rc = SUCCESS;
    int i = 0;
    struct mqtt_packet *packet = NULL;
    struct conn_context *conn = NULL;

    map_t *clients_map = NULL;
    map_iter_t iter;

    if((clients_map = topic_get_clients_map(topic)) == NULL){
        DEBUG(100, "topic<%s> no clients subscribe\n", topic);
        return SUCCESS;
    }

    
    if((rc = gain_raw_publish_packet(topic,
                   message,  &packet)) != SUCCESS){
        ERROR("packet raw publish package failed\n");
        return rc;
    }

    iter = map_iter();

    while((conn = (struct conn_context*)map_next_val(clients_map, &iter)) != NULL){
        if(push_msg_to_conn(conn, packet) == 0){
            DEBUG(100,"publish <%s> message <%s> to client <%s> for <topic> %s\n", 
                publish_id, message, conn->client_id, topic);
        }
    }

  free_packet:
    destroy_mqtt_packet(packet);
    return rc;
}

int rpc_get_tonc_process(struct conn_context *conn, char *topic)
{
    int rc = SUCCESS;
    int i = 0;
    struct mqtt_packet *packet = NULL;
    int online = 0;
    map_t *clients_map = NULL;

    DEBUG(10, "mqtt connect process \n");

    if((clients_map = topic_get_clients_map(topic)) != NULL){
        online = map_get_count(clients_map);
    }

    DEBUG(100, "topic<%s> online count:%d\n", topic, online);

    
    if((rc = gain_raw_tonc_ack_packet(online, &packet)) != SUCCESS){
        ERROR("packet raw queyry topic online count package failed\n");
        return rc;
    }


    rc = write_mqtt_packet(conn, packet);
    destroy_mqtt_packet(packet);
    return rc;
} 

static int gain_raw_publish_packet(char *topic_name, char *msg, struct mqtt_packet **packet)
{
    int rc = SUCCESS;
    int msg_len = 0;
    int qos = 0;
    struct mqtt_packet *tmp = NULL;

    if((rc = malloc_mqtt_packet(&tmp)) != SUCCESS){
        return rc;
    }
    
    /* fill fix header*/
    tmp->command = MQTT_PUBLISH;
    tmp->fix_header = tmp->command |(qos << 1);

    /* get remaning length */
    tmp->remaining_length = 2 + strlen(topic_name); /* topic->name*/
    msg_len = strlen(msg);
    tmp->remaining_length += msg_len;

    /* fill tmp body*/
    tmp->body = (uint8_t *)malloc(sizeof(uint8_t) * tmp->remaining_length);
    if(NULL == tmp->body){
        ERROR("malloc space failed, %m\n");
        rc = ERR_NOMEM;
        goto out;
    }
    bzero(tmp->body, tmp->remaining_length);
    
    mqtt_write_string(tmp, topic_name, strlen(topic_name));
    mqtt_write_bytes(tmp, msg, msg_len);

    if(tmp->body_pos != tmp->remaining_length){
        DEBUG(100, "something wrong when fill publish package\n");
        //TODO
    }

    *packet = tmp;

    rc = SUCCESS;
  out:
    if(rc != SUCCESS && NULL != tmp){
        destroy_mqtt_packet(tmp);
    }
    return rc;
}

static int gain_raw_tonc_ack_packet(int online, struct mqtt_packet **packet)
{
    int rc = SUCCESS;
    int msg_len = 0;
    int qos = 0;
    struct mqtt_packet *tmp = NULL;

    if((rc = malloc_mqtt_packet(&tmp)) != SUCCESS){
        return rc;
    }
    
    /* fill fix header*/
    tmp->command = RPC_TONC_ACK;
    tmp->fix_header = tmp->command ;

    /* get remaning length */
    tmp->remaining_length = 4 ; /* topic->name*/

    /* fill tmp body*/
    tmp->body = (uint8_t *)malloc(sizeof(uint8_t) * tmp->remaining_length);
    if(NULL == tmp->body){
        ERROR("malloc space failed, %m\n");
        rc = ERR_NOMEM;
        goto out;
    }
    bzero(tmp->body, tmp->remaining_length);
    
    mqtt_write_uint32(tmp, online);

    *packet = tmp;

    rc = SUCCESS;
  out:
    if(rc != SUCCESS && NULL != tmp){
        destroy_mqtt_packet(tmp);
    }
    return rc;
}

static int push_msg_to_conn(struct conn_context *conn, struct mqtt_packet *packet)
{
    int rc = SUCCESS;

    CONN_REFER_INC(conn);
    rc =  write_mqtt_packet(conn, packet);
    CONN_REFER_DEC(conn);
    return rc;
}


