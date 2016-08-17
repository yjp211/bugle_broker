#ifndef _POOL_H_
#define _POOL_H_

#ifdef __APPLE__
#include <dispatch/dispatch.h>
#else
#include <semaphore.h>
#endif

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>


struct queue_item {
    struct conn_context *conn;
    struct mqtt_packet *packet;

    struct queue_item *next;
};

struct queue {
    int64_t length;

    struct queue_item *head;
    struct queue_item *tail;
};

struct rk_sema {
  #ifdef __APPLE__
    dispatch_semaphore_t sem;
  #else
    sem_t   sem;
  #endif
};

struct thread_pool {
    pthread_t *threads;
    int  count;
    struct rk_sema *sem;
    pthread_mutex_t lock;
    struct queue *data_queue;
    
    void (*handle)(struct conn_context *, struct mqtt_packet *);
};

int init_thread_pool(int count, 
    void (*handle)(struct conn_context *, struct mqtt_packet *),
        struct thread_pool **pool);
void deliver_packet_to_pool(struct conn_context *conn, struct mqtt_packet *packet, struct thread_pool *pool);

#endif
