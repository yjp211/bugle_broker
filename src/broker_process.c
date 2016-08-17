#include "include.h"
#include "common_process.h"
#include "broker_process.h"
#include "topic.h"


static int gain_raw_connack_packet(int result, struct mqtt_packet **packet);
static int gain_raw_suback_packet(uint16_t msg_id, uint8_t *qoss, int count, struct mqtt_packet **packet);
static int gain_raw_unsuback_packet(uint16_t msg_id, struct mqtt_packet **packet);


int mqtt_connect_process(struct conn_context *conn, int result)
{
        
    int rc = SUCCESS;
    char time_serie[TIME_SERIE_LEN];
    struct mqtt_packet *packet = NULL;

    DEBUG(10, "mqtt connect process \n");


    if((rc = gain_raw_connack_packet(result, &packet)) != SUCCESS){
        ERROR("packet raw connect ack package failed\n");
        return rc;
    }

    if(rc == SUCCESS && result == SUCCESS){
        set_conn_authed(conn);
        valid_conn_add_member(conn); // add connect to global valid client list   
    }

    rc = write_mqtt_packet(conn, packet);
    destroy_mqtt_packet(packet);
    return SUCCESS;
}


int mqtt_subscribe_process(struct conn_context *conn, uint16_t msg_id, struct mqtt_topic **sub_topics, int topic_count)
{


    int rc = SUCCESS;
    int i = 0;
    struct mqtt_packet *packet = NULL;
    uint8_t *qoss = NULL;
    
    if(topic_count <= 0){
        return SUCCESS;
    }

    if(!is_conn_auth(conn)){
        return ERR_NO_AUTH;
    }

    if((qoss = (uint8_t *)malloc(sizeof(uint8_t) * topic_count)) == NULL){
        ERROR("malloc faied, %m\n");
        rc = ERR_NOMEM;
        goto out;
    }

    for(i = 0; i < topic_count; i++){
        if((rc = conn_add_topic(conn, sub_topics[i]->name)) != SUCCESS){
            goto out;
        }
        qoss[i] = 0;
        topic_add_client(sub_topics[i]->name, conn);
    }

    
    if(rc == SUCCESS){
        if((rc = gain_raw_suback_packet(msg_id, qoss, topic_count, &packet)) != SUCCESS){
            ERROR("packet raw sub ack package failed\n");
            goto out;
        }
        rc = write_mqtt_packet(conn, packet);
    }
  out:
    if(qoss != NULL) free(qoss);
    destroy_mqtt_packet(packet);
    
    return rc;
}


int mqtt_unsubscribe_process(struct conn_context *conn, uint16_t msg_id, struct mqtt_topic **sub_topics, int topic_count)
{
    int rc = SUCCESS;
    int i = 0;
    struct mqtt_packet *packet = NULL;

    if(topic_count <= 0){
        return SUCCESS;
    }
    for(i = 0; i < topic_count; i++){
        if((rc = conn_remove_topic(conn, sub_topics[i]->name)) != SUCCESS){
            goto out;
        }
        topic_remove_client(sub_topics[i]->name, conn->client_id);
    }
    
    if((rc = gain_raw_unsuback_packet(msg_id, &packet)) != SUCCESS){
        ERROR("packet raw unsub ack package failed\n");
        goto out;
    }
    if((rc = write_mqtt_packet(conn, packet)) != SUCCESS){
        ERROR("send subscribe ack package failed\n");
        goto out;
    }

  out:
    destroy_mqtt_packet(packet);
    
    return rc;
}


static int gain_raw_connack_packet(int result, struct mqtt_packet **packet)
{
        
    int rc = SUCCESS;
    struct mqtt_packet *tmp = NULL;

    if((rc = malloc_mqtt_packet(&tmp)) != SUCCESS){
        return rc;
    }
    
    /* fill fix header*/
    tmp->command = MQTT_CONNACK;
    tmp->fix_header = tmp->command;

    /* get remaning length */
    tmp->remaining_length = 2; 

    /* fill packet body*/
    tmp->body = (uint8_t *)malloc(sizeof(uint8_t) * tmp->remaining_length);
    if(NULL == tmp->body){
        ERROR("malloc space failed, %m\n");
        rc = ERR_NOMEM;
        goto out;
    }
    bzero(tmp->body, tmp->remaining_length);
    /* write a byte, no use but reserve */
    mqtt_write_byte(tmp, 0); 

    /*write result */
    if(result < 0 || result > 255){
        result = ERR_SYS_ERROR;
    }
    mqtt_write_byte(tmp, result); 
    
    *packet = tmp;

    rc = SUCCESS;
    
  out:
    if(rc != SUCCESS && NULL != tmp){
        destroy_mqtt_packet(tmp);
    }
    return rc;
}

static int gain_raw_suback_packet(uint16_t msg_id, uint8_t *qoss, int count, struct mqtt_packet **packet)
{
    int rc = SUCCESS;
    struct mqtt_packet *tmp = NULL;

    if((rc = malloc_mqtt_packet(&tmp)) != SUCCESS){
        return rc;
    }
    
    /* fill fix header*/
    tmp->command = MQTT_SUBACK;
    tmp->fix_header = tmp->command;

    /* get remaning length */
    tmp->remaining_length = 2 + count; /* msg_id, qos count*/

    /* fill tmp body*/
    tmp->body = (uint8_t *)malloc(sizeof(uint8_t) * tmp->remaining_length);
    if(NULL == tmp->body){
        ERROR("malloc space failed, %m\n");
        rc = ERR_NOMEM;
        goto out;
    }
    bzero(tmp->body, tmp->remaining_length);
    
    mqtt_write_uint16(tmp, msg_id);
    mqtt_write_bytes(tmp, qoss, count);

    if(tmp->body_pos != tmp->remaining_length){
        DEBUG(100, "something wrong when fill sub ack package\n");
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

static int gain_raw_unsuback_packet(uint16_t msg_id, struct mqtt_packet **packet)
{
    int rc = SUCCESS;
    struct mqtt_packet *tmp = NULL;

    if((rc = malloc_mqtt_packet(&tmp)) != SUCCESS){
        return rc;
    }
    
    /* fill fix header*/
    tmp->command = MQTT_UNSUBACK;
    tmp->fix_header = tmp->command;

    /* get remaning length */
    tmp->remaining_length = 2; /* msg_id, qos count*/

    /* fill tmp body*/
    tmp->body = (uint8_t *)malloc(sizeof(uint8_t) * tmp->remaining_length);
    if(NULL == tmp->body){
        ERROR("malloc space failed, %m\n");
        rc = ERR_NOMEM;
        goto out;
    }
    bzero(tmp->body, tmp->remaining_length);
    
    mqtt_write_uint16(tmp, msg_id);

    if(tmp->body_pos != tmp->remaining_length){
        DEBUG(100, "something wrong when fill sub ack package\n");
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

