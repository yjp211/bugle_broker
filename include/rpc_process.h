#ifndef _RPC_PROCESS_H_
#define _RPC_PROCESS_H_

int rpc_publish_pure_process(char *publish_id, char *topic, char *message); 
int rpc_get_tonc_process(struct conn_context *conn, char *topic); 
#endif
