/** 
 * Copyright (c) 2014 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "map.h"

struct map_node_t {
    unsigned hash;
    char *key;
    void *value;
    map_node_t *next;
};


static unsigned map_hash(const char *str) {
    unsigned hash = 5381;
    while (*str) {
        hash = ((hash << 5) + hash) ^ *str++;
    }
    return hash;
}


static map_node_t *map_newnode(const char *key, void *value)
{
    map_node_t *node;

    node = malloc(sizeof(*node));
    if (!node){
        return NULL;
    }

    node->hash = map_hash(key);
    node->key = (char *)key;
    node->value = value;

    return node;
}


static int map_bucketidx(map_t *m, unsigned hash) {
  /* If the implementation is changed to allow a non-power-of-2 bucket count,
   * the line below should be changed to use mod instead of AND */
    return hash & (m->nbuckets - 1);
}


static void map_addnode(map_t *m, map_node_t *node) {
    int n = map_bucketidx(m, node->hash);
    node->next = m->buckets[n];
    m->buckets[n] = node;
}


static int map_resize(map_t *m, int nbuckets) {
    map_node_t *nodes, *node, *next;
    map_node_t **buckets;
    int i; 
    /* Chain all nodes together */
    nodes = NULL;
    i = m->nbuckets;
    while (i--) {
        node = (m->buckets)[i];
        while (node) {
            next = node->next;
            node->next = nodes;
            nodes = node;
            node = next;
        }
    }
    /* Reset buckets */
    buckets = realloc(m->buckets, sizeof(*m->buckets) * nbuckets);
    if (buckets != NULL) {
        m->buckets = buckets;
        m->nbuckets = nbuckets;
    }
    if (m->buckets) {
        memset(m->buckets, 0, sizeof(*m->buckets) * m->nbuckets);
        /* Re-add nodes to buckets */
        node = nodes;
        while (node) {

            next = node->next;
            map_addnode(m, node);
            node = next;
        }
    }
    /* Return error code if realloc() failed */
    return (buckets == NULL) ? -1 : 0;
}


static map_node_t **map_getref(map_t *m, const char *key) {
    unsigned hash = map_hash(key);
    map_node_t **next;
    if (m->nbuckets > 0) {
        next = &m->buckets[map_bucketidx(m, hash)];
        while (*next) {
        //if ((*next)->hash == hash && !strcmp((char*) (*next + 1), key)) {
            if ((*next)->hash == hash && !strcmp((*next)->key, key)) {
            return next;
        }
            next = &(*next)->next;
        }
    }
    return NULL;
}

int map_init(map_t *m, unsigned size, unsigned step)
{
    if(size == 0){
        size = 1;
    } 
    
    pthread_rwlock_init(&(m->lock), NULL);

    m->buckets = malloc(sizeof(map_node_t *) * size);
    if(NULL == m->buckets){
        return -1;
    }
    memset(m->buckets, 0, sizeof(map_node_t *) * size);

    m->nnodes = 0;
    m->nbuckets = size;
    m->step = step;
    return 0;
}



void map_deinit(map_t *m) {
    map_node_t *next, *node;
    int i;
    i = m->nbuckets;
    while (i--) {
        node = m->buckets[i];
        while (node) {
            next = node->next;
            free(node);
            node = next;
        }
    }
    pthread_rwlock_destroy(&(m->lock));
    free(m->buckets);
}


void *map_get(map_t *m, const char *key)
{
    if(NULL == key){
        return NULL;
    }

    pthread_rwlock_rdlock(&(m->lock));
    map_node_t **next = map_getref(m, key);
    pthread_rwlock_unlock(&(m->lock));
    
    return next ? (*next)->value : NULL;
}


int map_set(map_t *m, const char *key, void *value, int ig_conflict)
{

    if(NULL == key || NULL == value){
        return -1;
    }

    int new_szie ;
    map_node_t **next, *node;

    pthread_rwlock_wrlock(&(m->lock));
    
    /* Find & replace existing node */
    next = map_getref(m, key);
    if (next) {
        if(ig_conflict){
            (*next)->value = value;
            pthread_rwlock_unlock(&(m->lock));
            return 0;
        }else {
            pthread_rwlock_unlock(&(m->lock));
            return 1;
        }
    }

    /* Add new node */
    node = map_newnode(key, value);
    if (node == NULL){
        goto fail;
    }
    
    /* need resize? */
    if (m->nnodes >= m->nbuckets) {
        new_szie = (m->step > 0) ? (m->nbuckets + m->step) : (m->nbuckets << 1);
        if(map_resize(m, new_szie) != 0){
            goto fail;
        }
    }
    map_addnode(m, node);
    m->nnodes++;
    pthread_rwlock_unlock(&(m->lock));

    return 0;
  fail:
    if (node) free(node);
    pthread_rwlock_unlock(&(m->lock));

    return -1;
}


void *map_remove(map_t *m, const char *key)
{
    if(NULL == key){
        return NULL;
    }

    void *value =  NULL;
    map_node_t *node;
    
    pthread_rwlock_wrlock(&(m->lock));
    map_node_t **next = map_getref(m, key);
    if (next) {
        value = (*next)->value;
        node = *next;
        *next = (*next)->next;
        free(node);
        m->nnodes--;
    }
    pthread_rwlock_unlock(&(m->lock));
    return value;
}


int map_get_count(map_t *m){
    return m->nnodes;
}

map_iter_t map_iter(void) {
  map_iter_t iter;
  iter.bucketidx = -1;
  iter.node = NULL;
  return iter;
}


const char *map_next_key(map_t *m, map_iter_t *iter) {
  if (iter->node) {
    iter->node = iter->node->next;
    if (iter->node == NULL) goto nextBucket;
  } else {
    nextBucket:
    do {
      if (++iter->bucketidx >= m->nbuckets) {
        return NULL;
      }
      iter->node = m->buckets[iter->bucketidx];
    } while (iter->node == NULL);
  }
  return iter->node->key; 
}

const void *map_next_val(map_t *m, map_iter_t *iter) {
  if (iter->node) {
    iter->node = iter->node->next;
    if (iter->node == NULL) goto nextBucket;
  } else {
    nextBucket:
    do {
      if (++iter->bucketidx >= m->nbuckets) {
        return NULL;
      }
      iter->node = m->buckets[iter->bucketidx];
    } while (iter->node == NULL);
  }
  return iter->node->value;
}
#if 0
#include <strings.h>
#include <stdio.h>
struct test_t {
    char *data;
    int id;
};


int main(int argc, char **agrv)
{
    int rc = 0;
    int i = 0;
    map_t g_map;
    char key[100];
    char data[1024];
    struct test_t *tmp = NULL;

    printf("------>start\n");
    
    map_init(&g_map, 0, 0);

    for(i = 0; i < 4096; i++){
        bzero(key, 100);
        bzero(data, 1024);
        snprintf(key, 100, "K_%d", i);
        snprintf(data, 1024, "val_%d", i*i);
        tmp = malloc(sizeof(struct test_t));
        memset(tmp, 0, sizeof(struct test_t));
        tmp->data = strdup(data);
        tmp->id = i;
     //   printf("set %s:%s\n", key, data);
        map_set(&g_map, key, tmp);
    }



    printf("------>end\n");
    bzero(key, 100);
    strcpy(key, "K_401");
    tmp = (void *)map_get(&g_map, key);
    if(tmp != NULL){
        printf("key:%s, index:%d, value:%s\n",
                key, tmp->id, tmp->data);
    }
    printf("------>end22\n");

}
#endif
