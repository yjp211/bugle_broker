/** 
 * Copyright (c) 2014 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 */

#ifndef MAP_H
#define MAP_H

#include <string.h>

#define MAP_VERSION "0.1.0"

struct map_node_t;
typedef struct map_node_t map_node_t;

typedef struct {
    map_node_t **buckets;
    unsigned nbuckets;
    unsigned nnodes;
    unsigned step;
    pthread_rwlock_t lock;

} map_t;

typedef struct {
    unsigned bucketidx;
    map_node_t *node;
} map_iter_t;



int map_init(map_t *m, unsigned size, unsigned step);
void map_deinit(map_t *m);
void *map_get(map_t *m, const char *key);
int map_set(map_t *m, const char *key, void *value, int ig_conflict);
void *map_remove(map_t *m, const char *key);

int map_get_count(map_t *m);

map_iter_t map_iter(void);
const char *map_next_key(map_t *m, map_iter_t *iter);
const void *map_next_val(map_t *m, map_iter_t *iter);



#endif
