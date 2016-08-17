#include "include.h"
#include "common_handle.h"
#include "common_process.h"
#include "rpc_handle.h"
#include "rpc_process.h"

#define RPC_HANDLE_COUNT  4


struct mqtt_handle rpc_handles[RPC_HANDLE_COUNT] = {
    {DISCONNECT, "disconnect", common_disconnect_handle},
    {PINGREQ, "ping request", common_pingreq_handle},
    {RPC_PURE_PUB, "publish pure message", rpc_pure_publish_handle},
    {RPC_TONC, "query topic online count", rpc_get_tonc_handle},
};


void rpc_handle(struct conn_context *conn, struct mqtt_packet *packet)
{
    int i;
    struct mqtt_handle *handle = NULL;

    for(i = 0; i < RPC_HANDLE_COUNT; i++){
        handle = &rpc_handles[i];
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

    if(i == RPC_HANDLE_COUNT){
        ERROR("undefined rpc protocol: 0x%x\n", packet->command);
    }
}
/**
 * publish pure mesg: 协议格式
 * 可变头:
 * payload:
 *      1. publish uuid <string>
 *      2. publish topic<string>
 *      3. message body <string>
 *
 */
int rpc_pure_publish_handle(struct conn_context *conn, struct mqtt_packet *packet)
{
    int rc = SUCCESS;
    int i = 0;
    char *pub_uuid = NULL;
    char *topic = NULL;
    char *body = NULL;

    DEBUG(10, "rpc publish pure message handle\n");

    if((rc = mqtt_read_string(packet, &pub_uuid)) != SUCCESS){
        ERROR("error read publish uuid\n");
        goto out;         
    }
    DEBUG(100, "publish uuid: [%s]\n", pub_uuid);
    
    if((rc = mqtt_read_string(packet, &topic)) != SUCCESS){
        ERROR("error read publish topic\n");
        goto free_uuid;         
    }
    DEBUG(100, "publish topic: [%s]\n", topic);

    if((rc = mqtt_read_string(packet, &body)) != SUCCESS){
        ERROR("error read publish message\n");
        goto free_topic;         
    }
    DEBUG(100, "publish message: [%s]\n", body);

    rc = rpc_publish_pure_process(pub_uuid, topic, body);

  free_body:
    if(body) free(body);
  free_topic:
    if(topic) free(topic);
  free_uuid:
    if(pub_uuid) free(pub_uuid);
  out:
    return rc;

}

/**
 * query topic online count: 协议格式
 * 可变头:
 * payload:
 *      1. topic <string>
 *
 */
int rpc_get_tonc_handle(struct conn_context *conn, struct mqtt_packet *packet)
{
    int rc = SUCCESS;
    int i = 0;
    char *topic = NULL;

    DEBUG(10, "rpc get topic online count message handle\n");

    
    if((rc = mqtt_read_string(packet, &topic)) != SUCCESS){
        ERROR("error read topic\n");
        goto out;         
    }
    DEBUG(100, "query online count topic: [%s]\n", topic);

    rc = rpc_get_tonc_process(conn, topic);

    if(topic) free(topic);
  out:
    return rc;

}

