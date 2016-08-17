#include "include.h"
#include "common_handle.h"
#include "broker_handle.h"
#include "broker_process.h"

#define BROKER_HANDLE_COUNT  5


struct mqtt_handle broker_handles[BROKER_HANDLE_COUNT] = {
    {CONNECT, "connect(auth)", mqtt_conncet_handle},
    {DISCONNECT, "disconnect", common_disconnect_handle},
    {PINGREQ, "ping request", common_pingreq_handle},
    {MQTT_SUBSCRIBE, "subscribe", mqtt_subscribe_handle},
    {MQTT_UNSUBSCRIBE, "unsubscribe", mqtt_unsubscribe_handle},
 //   {MQTT_PUBACK, "puback", mqtt_puback_handle},
};


void broker_handle(struct conn_context *conn, struct mqtt_packet *packet)
{
    int i;
    struct mqtt_handle *handle = NULL;

    for(i = 0; i < BROKER_HANDLE_COUNT; i++){
        handle = &broker_handles[i];
        if(handle->command == packet->command){
            DEBUG(100, "------>====thread:<%u> process this handle <%s>\n", (unsigned int)pthread_self(), handle->name);
            if(handle->command != DISCONNECT){
                CONN_REFER_INC(conn);
            }
            handle->handle(conn, packet);
            if(handle->command != DISCONNECT){
                CONN_REFER_DEC(conn);
            }
            break;
        }
    }

    if(i == BROKER_HANDLE_COUNT){
        ERROR("undefined mqtt protocol: 0x%x\n", packet->command);
    }
}


/**
 * connect协议格式
 * 可变头:
 *      1. 协议名称, 前2字节为名称字串长度的lsb和msb。当前协议名称固定为MQlsdp
 *      2. 协议版本号,1 byte
 *      3. 连接标志, 1 字节.8位(0-7)的含义如下
 *          0: 保留
 *          1: clean session
 *          2: will flag
 *        3-4: will qos
 *          5: will retain
 *          6: password flag
 *          7: username flag
 *      4. keep alive timer，两字节
 * payload:
 *      1. client id，UTF字串
 *      2. will flag启用
 *          will topic 字串
 *          will message 字串
 *      3. user name flag 启用
 *          username 字串
 *      4. passwor flag 启用
 *          password 字串
 */
int mqtt_conncet_handle(struct conn_context *conn, struct mqtt_packet *packet)
{
    int rc = SUCCESS;
    char *protocol_name = NULL;
    uint8_t protocol_version;
    uint8_t connect_flags;
    uint16_t keep_alive;
    char *client_id = NULL;
    char *will_topic = NULL, *will_message = NULL;
    char *user_name = NULL, *passwd = NULL;
    uint8_t will_flag, will_retain, will_qos, clean_start, uname_exist, paswd_exist;
    
    
    DEBUG(10, "mqtt connect handle\n");

    /* get protocol name */
    if(( rc = mqtt_read_string(packet, &protocol_name)) != SUCCESS){
        ERROR("error parse protocol name\n ");
        goto out;
    }
    DEBUG(100, "protocol name:%s\n", protocol_name);
    if(strcmp(protocol_name, PROTOCOL_NAME_MQTT) != 0 &&
            strcmp(protocol_name, PROTOCOL_NAME_MQISDP) != 0){
        ERROR("not support this protocol name:%s\n", protocol_name);
        rc = ERR_PROTOCOL;
        goto free_protocol;
    }

    /* get protocol  version*/
    if((rc = mqtt_read_byte(packet, &protocol_version)) != SUCCESS){
        ERROR("error read protocol version\n");
        goto free_protocol;
    }
    DEBUG(100, "protocol version:%d\n", protocol_version);
    if(protocol_version != PROTOCOL_VERSION_MQTT &&
            protocol_version != PROTOCOL_VERSION_MQISDP){
        ERROR("not suport this protocol <%d>\n", protocol_version);
        //FIXME need to send back connack
        rc = ERR_PROTOCOL;
        goto free_protocol;

    }

    /* get connect flags*/
    if((rc = mqtt_read_byte(packet, &connect_flags)) != SUCCESS){
        ERROR("error read connect flags\n");
        goto free_protocol;
    }
    clean_start = connect_flags & 0x02;
    will_flag = connect_flags & 0x04;
    will_qos = (connect_flags & 0x18) >> 2;
    will_retain = connect_flags & 0x20;
    uname_exist = connect_flags & 0x80;
    paswd_exist = connect_flags & 0x40;

    DEBUG(100, "connect flags: uname[%d], paswd[%d], will retain[%d]," 
             "will qos[%d], will_flag[%d], clean session[%d]\n", uname_exist,
             paswd_exist, will_retain, will_qos, will_flag, clean_start);

    /* get keep alive timer*/
    if((rc = mqtt_read_uint16(packet, &keep_alive)) != SUCCESS){
        ERROR("error read keep alive timer\n");
        goto free_protocol;
    }
    DEBUG(100, "client keep alive timer: %d\n", keep_alive);

    /* get client id */
    if((rc = mqtt_read_string(packet, &client_id))){
        ERROR("error read client id\n");
        goto free_protocol;
    }
    //FIXME - need to check for valid name,
    //FIXME - need to check fo same client id existing
    DEBUG(100, "client id: %s, socket fd: %d\n", client_id, conn->socket_fd);
    if(NULL != conn->client_id){
        free(conn->client_id);
        conn->client_id = NULL;
    }
    
    //用socket_fd替代clientid
    //conn->client_id = client_id; //this will free in destroy_connection function
    free(client_id);
    if(asprintf(&(conn->client_id), "%d", conn->socket_fd) <=0){
        ERROR("asprintf failed, %m\n");
        goto free_protocol;
    }
    

    /* get will info */
    if(will_flag){
        /* get will topic*/
        if((rc = mqtt_read_string(packet, &will_topic))){
            ERROR("error read will topic\n");
            goto free_protocol;
        }
        /* get will message*/
        if((rc = mqtt_read_string(packet, &will_message))){
            ERROR("error read will message\n");
            goto free_topic;
        }
        DEBUG(100, "will topic:%s, will message:%s\n", will_topic, will_message);
        //FIXME 
    }

    /* get auth info */
    if(uname_exist && paswd_exist){
        if((rc = mqtt_read_string(packet, &user_name))){
            ERROR("error read user name\n");
            goto free_message;
        }
        DEBUG(100, "user name: %s\n", user_name);
        if(NULL != conn->user_name){
            free(conn->user_name);
        }
        conn->user_name = user_name; //this will free in destroy_connection function

        if((rc = mqtt_read_string(packet, &passwd))){
            ERROR("error read password\n");
            goto free_message;
        }
        DEBUG(100, "password: %s\n", passwd);
        if(NULL != conn->passwd){
            free(conn->passwd);
        }
        conn->passwd = passwd;
    }else {
        rc = ERR_REJECT;
    }
    
  free_message:
    if(will_message) free(will_message);
  free_topic:
    if(will_topic) free(will_topic);
  free_protocol:
    if(protocol_name) free(protocol_name);
  out:
    mqtt_connect_process(conn, rc);
    return rc;
}

#if 0
/**
 * publish ack协议格式
 * 可变头:
 *      1. 用两个字节代表msg id 
 * payload:
 *      null
 */
int mqtt_puback_handle(struct conn_context *conn, struct mqtt_packet *packet)
{
    int rc = SUCCESS;
    uint16_t msg_id = 0;
    
    DEBUG(10, "mqtt puback handle\n");

    if((rc = mqtt_read_uint16(packet, &msg_id)) != SUCCESS){
        ERROR("error read message id\n");
        return rc;
    }

    rc = mqtt_publish_ack_process(conn, msg_id);

    return rc;
}
#endif

/**
 * subscribe 协议格式
 * 可变头:
 *      1. uint16(两字节长度)的message id
 * payload:
 *      由多个<topic+qos>构成
 *      topic: utf8字串
 *      qos: 一个字节
 *
 */
int mqtt_subscribe_handle(struct conn_context *conn, struct mqtt_packet *packet)
{
    int rc = SUCCESS;
    uint16_t msg_id;
    char *sub;
    uint8_t qos;
    struct mqtt_topic *topic = NULL;
    struct mqtt_topic **sub_topics = NULL, **tmp=NULL;
    int default_len = 5, step = 5;
    int total_len = 0;
    int topic_count = 0;

    DEBUG(10, "mqtt subscribe handle\n");

    /* get message id*/
    if((rc = mqtt_read_uint16(packet, &msg_id)) != SUCCESS){
        ERROR("error read message id\n");
        goto out;
    }
    DEBUG(100, "subscribe message id: [%d]\n", msg_id);

    total_len = default_len;
    sub_topics = (struct mqtt_topic **)malloc(total_len * sizeof(struct mqtt_topic*));
    if(sub_topics == NULL){
        ERROR("error malloc mem failed, %m\n");
        rc = ERR_NOMEM;
        goto out;
    }

    while(packet->body_pos < packet->remaining_length){
        qos = 0;
        sub = NULL;
        topic = NULL;

        if((rc = mqtt_read_string(packet, &sub)) != SUCCESS){
            ERROR("error read sub topic\n");
            goto free_topics;         
        }
        DEBUG(100, "subscribe topic: [%s]\n", sub);
        
        if((rc = mqtt_read_byte(packet, &qos)) != SUCCESS){
            ERROR("error read sub topic qos\n");
            free(sub);
            goto free_topics;         
        }
        DEBUG(100, "subscribe topic qos: [%d]\n", qos);

        if(malloc_mqtt_topic(&topic) != SUCCESS ){
            free(sub);
            goto free_topics;
        }
        topic->name = sub;
        topic->qos = qos;
        if(topic_count + 1 > total_len){
            total_len += step;
            tmp = (struct mqtt_topic **)realloc(sub_topics, total_len * sizeof(struct mqtt_topic*));
            if(!tmp){
                ERROR("realloc space failed, %m\n");
                free(sub);
                free(topic);
                rc = ERR_NOMEM;
                goto free_topics;
            }
            sub_topics = tmp;
        }
        sub_topics[topic_count] = topic;
        topic_count++;
    }

    rc = mqtt_subscribe_process(conn, msg_id, sub_topics, topic_count);

  free_topics:
    if(sub_topics){
        int i = 0;
        for(i = 0; i < topic_count; i++){
            topic = sub_topics[i];
            //DEBUG(100, "---->topic:[%s], qos:[%d]\n", topic->name, topic->qos);
            destroy_mqtt_topic(topic);
        } 
        free(sub_topics);
        sub_topics = NULL;
    }
  out:
    return rc;
}


/**
 * unsubscribe 协议格式
 * 可变头:
 *      1. uint16(两字节长度)的message id
 * payload:
 *      由多个<topic+qos>构成
 *      topic: utf8字串
 *      qos: 一个字节
 *
 */
int mqtt_unsubscribe_handle(struct conn_context *conn, struct mqtt_packet *packet)
{
    int rc = SUCCESS;
    uint16_t msg_id;
    char *sub;
    uint8_t qos;
    struct mqtt_topic *topic = NULL;
    struct mqtt_topic **sub_topics = NULL, **tmp=NULL;
    int default_len = 5, step = 5;
    int total_len = 0;
    int topic_count = 0;

    DEBUG(10, "mqtt unsubscribe handle\n");

    /* get message id*/
    if((rc = mqtt_read_uint16(packet, &msg_id)) != SUCCESS){
        ERROR("error read message id\n");
        goto out;
    }
    DEBUG(100, "unsubscribe message id: [%d]\n", msg_id);

    total_len = default_len;
    sub_topics = (struct mqtt_topic **)malloc(total_len * sizeof(struct mqtt_topic*));
    if(sub_topics == NULL){
        ERROR("error malloc mem failed, %m\n");
        rc = ERR_NOMEM;
        goto out;
    }

    while(packet->body_pos < packet->remaining_length){
        qos = 0;
        sub = NULL;
        topic = NULL;

        if((rc = mqtt_read_string(packet, &sub)) != SUCCESS){
            ERROR("error read sub topic\n");
            goto free_topics;         
        }
        DEBUG(100, "unsubscribe topic: [%s]\n", sub);
        
        if(malloc_mqtt_topic(&topic) != SUCCESS ){
            free(sub);
            goto free_topics;
        }
        topic->name = sub;
        if(topic_count + 1 > total_len){
            total_len += step;
            tmp = (struct mqtt_topic **)realloc(sub_topics, total_len * sizeof(struct mqtt_topic*));
            if(!tmp){
                ERROR("realloc space failed, %m\n");
                free(sub);
                free(topic);
                rc = ERR_NOMEM;
                goto free_topics;
            }
            sub_topics = tmp;
        }
        sub_topics[topic_count] = topic;
        topic_count++;
    }

    rc = mqtt_unsubscribe_process(conn, msg_id, sub_topics, topic_count);

  free_topics:
    if(sub_topics){
        int i = 0;
        for(i = 0; i < topic_count; i++){
            topic = sub_topics[i];
            //DEBUG(100, "---->topic:[%s], qos:[%d]\n", topic->name, topic->qos);
            destroy_mqtt_topic(topic);
        } 
        free(sub_topics);
        sub_topics = NULL;
    }
   out:
    return rc;
}

