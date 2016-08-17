#include "include.h"
#include "common_process.h"

extern struct config_t g_config;

static int relese_connect(struct conn_context *conn)
{
    int rc = SUCCESS;
    int try_count = 0;
    
    while(CONN_HAVE_REFER(conn)) {
       sleep(g_config.check_refer_interval); 
       try_count++;
       if(try_count == g_config.check_refer_count){
           if(g_config.check_refer_timeout_action == 0){
               break;
           } else if(g_config.check_refer_timeout_action == 1){
               return rc;
           } else {
               return rc;
           }
       }
    }
    destroy_connect_context(conn);
    conn = NULL;
    
    return rc;

}

int common_disconnect_process(struct conn_context *conn)
{
    if(is_conn_auth(conn)){
        valid_conn_remove_member(conn);
    }
    
    return relese_connect(conn);
}

int common_ping_process(struct conn_context *conn)
{
        
    int rc = SUCCESS;
    struct mqtt_packet *packet = NULL;

    if((rc = malloc_mqtt_packet(&packet)) != SUCCESS){
        return rc;
    }
    
    /* fill fix header*/
    packet->command = PINGRESP;
    packet->fix_header = packet->command;

    /* get remaning length */
    packet->remaining_length = 0; 

    rc = write_mqtt_packet(conn, packet);
    
    destroy_mqtt_packet(packet);
    
    return SUCCESS;
}

