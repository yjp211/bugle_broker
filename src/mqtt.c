#include <evws/evws.h>
#include "include.h"
#include "mqtt.h"

#define MOSQ_MSB(A) (uint8_t)((A & 0xFF00) >> 8)
#define MOSQ_LSB(A) (uint8_t)(A & 0x00FF)

#define MOSQ_ASB(A) (uint8_t)((A & 0xFF000000) >> 24)
#define MOSQ_BSB(A) (uint8_t)((A & 0x00FF0000) >> 16)
#define MOSQ_CSB(A) (uint8_t)((A & 0x0000FF00) >> 8)
#define MOSQ_DSB(A) (uint8_t)(A & 0x000000FF)

int malloc_mqtt_packet(struct mqtt_packet **packet)
{
    struct mqtt_packet *tmp;
    if((tmp = (struct mqtt_packet*)malloc(sizeof(struct mqtt_packet))) == NULL){
        ERROR("malloc %d space failed, %m", (int)sizeof(struct mqtt_packet));
        return ERR_NOMEM;
    }

    tmp->fix_header = 0;
    tmp->command = 0;

    tmp->remaining_length = 0;
    tmp->body = NULL;
    tmp->body_pos = 0;
    
    tmp->head_is_read = 0;
    tmp->length_is_read =0;
    tmp->multiplier = 1;

    *packet = tmp;

    return SUCCESS;
}

int destroy_mqtt_packet(struct mqtt_packet *packet)
{
    if(!packet){
        return SUCCESS;
    }
    
    if(NULL != packet->body){
        free(packet->body);
    }

    free(packet);

    return SUCCESS;
}

int malloc_mqtt_topic(struct mqtt_topic **topic)
{
    struct mqtt_topic *tmp;
    if((tmp = (struct mqtt_topic*)malloc(sizeof(struct mqtt_topic))) == NULL){
        ERROR("malloc %d space failed, %m", (int)sizeof(struct mqtt_topic));
        return ERR_NOMEM;
    }
    tmp->name = NULL;
    tmp->msg = NULL;
    tmp->msg_len = 0;

    tmp->msg_id = -1; 

    tmp->qos = 0;
    tmp->dup = 0;
    tmp->retain = 0;

    tmp->sub_head = NULL;
    tmp->sub_count = 0;
    
    tmp->next = NULL;
    
    *topic = tmp;
    return SUCCESS;
}

int destroy_mqtt_topic(struct mqtt_topic *topic)
{
    struct sub_leaf *tmp, *next;

    if(!topic){
        return SUCCESS;
    }
    
    if(NULL != topic->name){
        free(topic->name);
    }
    if(NULL != topic->msg){
        free(topic->msg);
    }

    tmp = topic->sub_head;
    while(NULL != tmp){
        next = tmp->next;
        free(tmp); /* should't free sub_leaf->conn point*/
        tmp = next;
    }

    /* next point do't free here*/
    free(topic);

    return SUCCESS;
}

struct mqtt_topic *simple_dup_mqtt_topic(struct mqtt_topic *topic){
    struct mqtt_topic *tmp = NULL;

    if(malloc_mqtt_topic(&tmp) != SUCCESS){
        return NULL;
    }
    
    if(NULL != topic->name){
        tmp->name = strdup(topic->name);
    }
    if(NULL != topic->msg){
        tmp->msg = strdup(topic->msg);
    }

    tmp->msg_len = topic->msg_len;
    tmp->msg_id = topic->msg_id;

    tmp->qos = topic->qos;
    tmp->dup = topic->dup;
    tmp->retain = topic->retain;

    return tmp;
    
}

int mqtt_read_byte(struct mqtt_packet *packet, uint8_t *byte)
{
    if(packet->body_pos + 1 > packet->remaining_length){
        return ERR_PROTOCOL;
    }

    *byte = packet->body[packet->body_pos];
    packet->body_pos++;

    return SUCCESS;
}

int mqtt_read_uint16(struct mqtt_packet *packet, uint16_t *len)
{
    uint8_t msb, lsb;

    if(packet->body_pos + 2 > packet->remaining_length){
        return ERR_PROTOCOL;
    }

    msb = packet->body[packet->body_pos];
    packet->body_pos++;
    lsb = packet->body[packet->body_pos];
    packet->body_pos++;

    *len = (msb<<8) + lsb;

    return SUCCESS;
}

int mqtt_read_uint32(struct mqtt_packet *packet, uint32_t *len)
{
    uint8_t a,b,c,d;

    if(packet->body_pos + 4 > packet->remaining_length){
        return ERR_PROTOCOL;
    }

    a = packet->body[packet->body_pos];
    packet->body_pos++;
    b = packet->body[packet->body_pos];
    packet->body_pos++;
    c = packet->body[packet->body_pos];
    packet->body_pos++;
    d = packet->body[packet->body_pos];
    packet->body_pos++;

    *len = (a<<24) + (b<<16) + (c<<8) + d;

    return SUCCESS;
}
int mqtt_read_string(struct mqtt_packet *packet, char **str)
{
    uint16_t len;

    if(mqtt_read_uint16(packet, &len)){
        return ERR_PROTOCOL;
    }

    if(packet->body_pos + len > packet->remaining_length){
        return ERR_PROTOCOL;
    }

    if((*str =  (char *)malloc(sizeof(char) * (len + 1))) == NULL){
        return ERR_NOMEM;
    }
    //bzero(*str, len);
    bzero(*str, len + 1);
    memcpy(*str, &(packet->body[packet->body_pos]), len);
    packet->body_pos += len;

    return SUCCESS;
}

int mqtt_read_bytes(struct mqtt_packet *packet, void *bytes, uint32_t count)
{
    if(packet->body_pos + count > packet->remaining_length){
        return ERR_PROTOCOL;
    }

    memcpy(bytes, &(packet->body[packet->body_pos]), count);
    packet->body_pos += count;

    return SUCCESS;
}


void mqtt_write_byte(struct mqtt_packet *packet, uint8_t byte)
{
    packet->body[packet->body_pos] = byte;
    packet->body_pos++;
}

void mqtt_write_uint16(struct mqtt_packet *packet, uint16_t len)
{
    mqtt_write_byte(packet, MOSQ_MSB(len));
    mqtt_write_byte(packet, MOSQ_LSB(len));
}

void mqtt_write_uint32(struct mqtt_packet *packet, uint32_t len)
{
    mqtt_write_byte(packet, MOSQ_ASB(len));
    mqtt_write_byte(packet, MOSQ_BSB(len));
    mqtt_write_byte(packet, MOSQ_CSB(len));
    mqtt_write_byte(packet, MOSQ_DSB(len));
}

void mqtt_write_string(struct mqtt_packet *packet, const char *str, uint16_t length)
{
    mqtt_write_uint16(packet, length);
    mqtt_write_bytes(packet, str, length); 
}

void mqtt_write_bytes(struct mqtt_packet *packet, const void *bytes, uint32_t count)
{
    memcpy(&(packet->body[packet->body_pos]), bytes, count);
    packet->body_pos += count;
}

void read_mqtt_packet(struct conn_context *conn, struct thread_pool *pool)
{
    int len = 0;
    uint8_t byte = 0;
    uint8_t digit = 0;
    struct mqtt_packet *packet = NULL;
    
    while(1){
        packet = conn->packet;
        
        //packet为NULL，表示一个新包
        if(NULL == packet){
            if(malloc_mqtt_packet(&packet) != SUCCESS){
                ERROR("maolloc space failed\n");
                return;
            }
            conn->packet =  packet;
        }

        //未读取头部
        if(!packet->head_is_read){
            len = evbuffer_remove(conn->buf, &byte, 1);

            if(len == 0){//表示没有数据可读
                return;
            }else if(len < 0) {
                ERROR("read mqtt package head failed, %m\n");
                destroy_mqtt_packet(packet);
                conn->packet = NULL;
                return;
            }
            packet->fix_header = byte;
            packet->command = byte & 0xF0;//command永远大于0
            packet->head_is_read = 1;
        }

        //还没有读取remaing length，使用额外一个变量进行标记
        if(!packet->length_is_read) {
            do{
                len = evbuffer_remove(conn->buf, &digit, 1);
                if(len == 0) { //表示没有数据可读
                    return;
                }else if(len < 0){
                    ERROR("read mqtt package remaing lenght failed, %m\n");
                    destroy_mqtt_packet(packet);
                    conn->packet = NULL;
                    return;
                }
                packet->remaining_length += (digit & 127) * (packet->multiplier);
                packet->multiplier *= 128;
            }while((digit & 128) != 0);

            packet->length_is_read = 1;
            if(packet->remaining_length > 0 ){
                packet->body = (uint8_t *)malloc(
                        sizeof(uint8_t)* packet->remaining_length);
                if(NULL == packet->body){
                    ERROR("malloc %d space failed, %m\n", packet->remaining_length);
                    destroy_mqtt_packet(packet);
                    conn->packet = NULL;
                    return;
                }
                packet->body_pos = 0;
            }
        }

        if(packet->remaining_length > 0 && 
                packet->body_pos < packet->remaining_length){
            while(packet->body_pos < packet->remaining_length){
                len = evbuffer_remove(conn->buf, 
                        packet->body + (int)packet->body_pos, 
                        packet->remaining_length - packet->body_pos);
                if(len == 0){//没有数据可读
                    return;
                }else if(len < 0){
                    ERROR("read mqtt package remaing body failed, %m\n");
                    destroy_mqtt_packet(packet);
                    conn->packet = NULL;
                    return;
                }
                packet->body_pos += len;
            }
        }
        
        //全部数据已读完
        if(packet->body_pos == packet->remaining_length){
            //忽略客户端发送的连接断开协议，由socekt error触发disconnect调用，
            //否则多点触发在多线程下会有问题
            if(packet->command == DISCONNECT){
                destroy_mqtt_packet(packet);
            }else{
                packet->body_pos = 0; //将pos置回0
                deliver_packet_to_pool(conn, packet, pool);
            }
            conn->packet = NULL; //不要free packet，进入下一个数据包读取
        }
        continue;
    }
}


int write_raw_socket(int fd, const char *buf, int len)
{

    int have_write = 0;
    int wirte_length = 0;
    int remaining_length = 0;

    remaining_length = len;

    while(remaining_length > 0){
        wirte_length = write(fd, buf + have_write, remaining_length);
        if(wirte_length > 0){
            have_write += wirte_length;
            remaining_length -= wirte_length;
        }else{
            if(errno == EAGAIN){
                break;
            }else{
                ERROR("write mqtt package remaining body failed, %m\n");
                return ERR_SYS_FAILED;
            }
        }
    }
    return SUCCESS;
}

int write_raw_websocket(struct evwsconn *wsctx, const char *buf, int len)
{
    DEBUG(100,"----->wirite websocket length:%d !!!\n", len);
    evwsconn_send_message(wsctx, EVWS_DATA_BINARY, (const unsigned char*)buf, len);
    //evwsconn_send_message(wsctx, EVWS_DATA_BINARY, buf, len);
    return SUCCESS;
}

int write_mqtt_packet(struct conn_context *conn, struct mqtt_packet *packet)
{
    int rc = SUCCESS;
    uint8_t digit;
    uint32_t length = 0;
    
    int max_len = 0;
    char *buf = NULL;
    int qos = 0;
    
    DEBUG(100,"----->wirite packet:%x !!!\n", packet->command);

    if(NULL == conn ||
            !is_conn_active(conn)){
        return ERR_CONN_BREAK;
    }

    max_len = 1 + packet->remaining_length +  100;
    if((buf = malloc(sizeof(char) * max_len)) == NULL){
        ERROR("malloc failed,%m\n");
        return ERR_NOMEM;
    }
    memset(buf, 0, max_len*sizeof(char));

    buf[qos++] = packet->fix_header;

    /* write remaining length*/
    length = packet->remaining_length;
    do{
        digit = length % 128;
        length = length / 128;
        if(length > 0){
            digit = digit | 0x80;
        }
        buf[qos++] = digit;
    }while(length > 0);

    memcpy(buf+qos, packet->body, packet->remaining_length);
    qos += packet->remaining_length;


    pthread_mutex_lock(&(conn->write_lock));
    if(conn->conn_type == CONN_TYPE_TCP){
        rc = write_raw_socket(conn->socket_fd, buf, qos);
    }else if(conn->conn_type == CONN_TYPE_WEBSOCKETS){
        rc = write_raw_websocket(conn->wsctx, buf, qos);
    }else {
	ERROR("Dangling pointer is happening right now. You should reconstitute your architect.\n");
	rc = ERR_SYS_FAILED;
    }
    pthread_mutex_unlock(&(conn->write_lock));

    free(buf);
    return rc;
}

