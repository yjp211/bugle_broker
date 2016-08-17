#ifndef _BROKER_PROCESS_H_
#define _BROKER_PROCESS_H_

int mqtt_connect_process(struct conn_context *conn, int ret);

int mqtt_subscribe_process(struct conn_context *conn, uint16_t msg_id, struct mqtt_topic **sub_topics, int stopic_count);
int mqtt_unsubscribe_process(struct conn_context *conn, uint16_t msg_id, struct mqtt_topic **sub_topics, int topic_count);

//int mqtt_publish_ack_process(struct conn_context *conn, uint16_t msg_id);

#endif
