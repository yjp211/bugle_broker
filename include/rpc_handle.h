#ifndef _RPC_HANDLE_H_
#define _RPC_HANDLE_H_


void rpc_handle(struct conn_context *conn, struct mqtt_packet *packet);


//int rpc_publish_online_handle(struct conn_context *conn, struct mqtt_packet *packet);
//int rpc_publish_offline_handle(struct conn_context *conn, struct mqtt_packet *packet);

//int rpc_kickoff_client_handle(struct conn_context *conn, struct mqtt_packet *packet);

int rpc_pure_publish_handle(struct conn_context *conn, struct mqtt_packet *packet);
int rpc_get_tonc_handle(struct conn_context *conn, struct mqtt_packet *packet);
#endif
