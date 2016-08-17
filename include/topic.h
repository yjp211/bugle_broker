#ifndef _TOPIC_H_
#define _TOPIC_H_

#include "map.h"
#include "conn.h"


int init_topics_map(int size, int step);

int topic_add_client(const char *topic, struct conn_context *conn);
int topic_remove_client(const char *topic, const char *client_id);
int topic_get_clients_count(const char *topic, int *count);
map_t *topic_get_clients_map(const char *topic);


#endif
