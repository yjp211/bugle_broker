#ifndef _MQTT_H_
#define _MQTT_H_

#include <stdint.h>

#include "conn.h"
#include "pool.h"

#define PROTOCOL_NAME_MQTT "MQTT"
#define PROTOCOL_VERSION_MQTT 4

#define PROTOCOL_NAME_MQISDP "MQIsdp"
#define PROTOCOL_VERSION_MQISDP 3

#define MQTT_ID_MAX_LENGTH 23

/* Message types */
#define CONNECT 0x10
#define PINGREQ 0xC0
#define PINGRESP 0xD0
#define DISCONNECT 0xE0

#define MQTT_CONNACK 0x20
#define MQTT_PUBLISH 0x30
#define MQTT_PUBACK 0x40
#define MQTT_PUBREC 0x50
#define MQTT_PUBREL 0x60
#define MQTT_PUBCOMP 0x70
#define MQTT_SUBSCRIBE 0x80
#define MQTT_SUBACK 0x90
#define MQTT_UNSUBSCRIBE 0xA0
#define MQTT_UNSUBACK 0xB0

#define RPC_PUBONL 0x20//推送即时消息
#define RPC_PUBOFFL 0x30//推送离线消息
#define RPC_KICKOFF 0x40//踢用户下线
#define RPC_PURE_PUB 0x50//推送消息 不含推送列表
#define RPC_TONC 0x60//统计在线用户
#define RPC_TONC_ACK 0x70//统计在线用户，返回包

struct mqtt_packet {
    uint8_t fix_header;
    uint8_t command;

    uint32_t remaining_length;
    uint8_t *body;
    uint32_t body_pos;

    int head_is_read;
    int length_is_read;
    int multiplier;
};

struct mqtt_topic{
    char *name;
    char *msg;
    uint32_t msg_len;
    uint16_t msg_id;
    uint8_t retain;
    uint8_t dup;
    uint8_t qos;
    struct sub_leaf *sub_head;
    int sub_count;
    struct mqtt_topic *next;
};

struct sub_leaf{
    struct conn_context *conn;
    struct sub_leaf *next;
};

struct offline_msg{
    char *publish_id;
    char *message;
};

struct mqtt_handle {
    uint8_t command;
    char *name;
    int (*handle)(struct conn_context *, struct mqtt_packet *);
};


/*----------*/
int malloc_mqtt_packet(struct mqtt_packet **packet);
int destroy_mqtt_packet(struct mqtt_packet *packet);

/*----------*/
int malloc_mqtt_topic(struct mqtt_topic **topic);
int destroy_mqtt_topic(struct mqtt_topic *topic);
struct mqtt_topic *simple_dup_mqtt_topic(struct mqtt_topic *topic);


/*----------*/
int mqtt_read_byte(struct mqtt_packet *packet, uint8_t *byte);

int mqtt_read_uint16(struct mqtt_packet *packet, uint16_t *len);

int mqtt_read_uint32(struct mqtt_packet *packet, uint32_t *len);

int mqtt_read_string(struct mqtt_packet *packet, char **str);

int mqtt_read_bytes(struct mqtt_packet *packet, void *bytes, uint32_t count);


void mqtt_write_byte(struct mqtt_packet *packet, uint8_t byte);

void mqtt_write_uint16(struct mqtt_packet *packet, uint16_t len);

void mqtt_write_uint32(struct mqtt_packet *packet, uint32_t len);

void mqtt_write_string(struct mqtt_packet *packet, const char *str, uint16_t length);

void mqtt_write_bytes(struct mqtt_packet *packet, const void *bytes, uint32_t count);

void read_mqtt_packet(struct conn_context *conn, struct thread_pool *pool);
int write_mqtt_packet(struct conn_context *conn, struct mqtt_packet *packet);

#endif
