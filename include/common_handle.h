#ifndef _COMMON_HANDLE_H_
#define _COMMON_HANDLE_H_

int common_disconnect_handle(struct conn_context *conn, struct mqtt_packet *packet);

int common_pingreq_handle(struct conn_context *conn, struct mqtt_packet *packet);
#endif
