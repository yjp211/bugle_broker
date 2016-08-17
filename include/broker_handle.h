#ifndef _BROKER_HANDLE_H_
#define _BROKER_HANDLE_H_


void broker_handle(struct conn_context *conn, struct mqtt_packet *packet);
int init_broker_process_pool(int thread_count);

int mqtt_conncet_handle(struct conn_context *conn, struct mqtt_packet *packet);

int mqtt_subscribe_handle(struct conn_context *conn, struct mqtt_packet *packet);

int mqtt_unsubscribe_handle(struct conn_context *conn, struct mqtt_packet *packet);

//int mqtt_puback_handle(struct conn_context *conn, struct mqtt_packet *packet);

#endif
