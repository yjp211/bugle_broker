#include <pthread.h>
#include "include.h"
#include "pool.h"
#include "mqtt.h"

static int init_queue_item(struct queue_item **item);
static int destroy_queue_item(struct queue_item *item);
static void init_queue(struct queue *q);
static void push_queue_item(struct queue *q, struct conn_context *conn, struct mqtt_packet *packet);
static struct queue_item *pop_queue_item(struct queue *q);
static inline void rk_sema_init(struct rk_sema *s, uint32_t value);
static inline void rk_sema_destroy(struct rk_sema *s);
static inline void rk_sema_wait(struct rk_sema *s);
static inline void rk_sema_post(struct rk_sema *s);
static void* work_thread_loop(struct thread_pool *tp);

int init_thread_pool(int count, 
    void (*handle)(struct conn_context *, struct mqtt_packet *),
        struct thread_pool **pool)
{
    int i = 0;
    struct thread_pool *tp = NULL;

    tp = (struct thread_pool*)malloc(sizeof(struct thread_pool));
    if(NULL == tp){
        ERROR("malloc %d space failed,%m\n",(int)sizeof(struct thread_pool));
        return ERR_NOMEM;
    }
    
    tp->handle = handle;

    if(count <= 1){
        count = 1;
    }
    tp->count = count;

    //初始化线程组
    tp->threads = (pthread_t *)malloc(sizeof(pthread_t) * count);
    if(NULL == tp->threads){
        ERROR("malloc %d space failed,%m\n",(int)sizeof(pthread_t) * count);
        return ERR_NOMEM;
    }

    //初始化数据队列
    tp->data_queue = (struct queue *)malloc(sizeof(struct queue));
    if(NULL == tp->data_queue){
        ERROR("malloc %d space failed,%m\n", (int)sizeof(struct queue));
        return ERR_NOMEM;
    }
    init_queue(tp->data_queue);
    
    //初始化互斥锁
    pthread_mutex_init(&tp->lock, NULL);

    //初始化条件信号量
    tp->sem = (struct rk_sema *)malloc(sizeof(struct rk_sema));
    if(NULL == tp->sem){
        ERROR("malloc %d space failed,%m\n",(int)sizeof(struct rk_sema));
        return ERR_NOMEM;
    }

    rk_sema_init(tp->sem, 0);

    //初始化线程
    for(i=0; i < count; i++){
        pthread_create(&(tp->threads[i]), NULL, (void *)work_thread_loop, (void *)tp);
    }

    *pool = tp;

    return SUCCESS;

}


void deliver_packet_to_pool(struct conn_context *conn, struct mqtt_packet *packet, struct thread_pool *pool){
    struct thread_pool *tp = pool;

    pthread_mutex_lock(&tp->lock);
    push_queue_item(tp->data_queue, conn, packet);
    //发送工作信号量
    rk_sema_post(tp->sem);
    pthread_mutex_unlock(&tp->lock);
}

static int init_queue_item(struct queue_item **item)
{
    struct queue_item *tmp;
    if((tmp = (struct queue_item*)malloc(sizeof(struct queue_item))) == NULL){
        ERROR("malloc %d space failed, %m", (int)sizeof(struct queue_item));
        return ERR_NOMEM;
    }

    tmp->conn = NULL;
    tmp->packet = NULL;

    *item = tmp;
    return SUCCESS;
}

static int destroy_queue_item(struct queue_item *item)
{
    if(!item){
        return SUCCESS;
    }
    free(item);

    return SUCCESS;
}

static void init_queue(struct queue *q)
{
    q->length = 0;
   
    q->head = NULL;
    q->tail = NULL;
}

static void push_queue_item(struct queue *q, struct conn_context *conn, struct mqtt_packet *packet)
{

    struct queue_item *item = NULL;
   
    if(init_queue_item(&item) != SUCCESS) {
        ERROR("init queue item failed\n");
        return;
    }

    item->conn = conn;
    item->packet = packet;
    
    if(q->length == 0){
        q->head = item;
        q->tail = item;
        item->next = NULL;
    }else {
        q->tail->next = item;
        q->tail = item;
    }
    q->length++;

}

static struct queue_item *pop_queue_item(struct queue *q)
{
    struct queue_item *item = NULL;
    if(q->length != 0){
        item = q->head;
        if(q->length == 1){
            q->head = NULL;
            q->tail = NULL;
        }else {
            q->head = q->head->next;
        }       
        q->length--;
    }
    return item;
};

static inline void rk_sema_init(struct rk_sema *s, uint32_t value)
{
  #ifdef __APPLE__
    dispatch_semaphore_t *sem = &s->sem;

    *sem = dispatch_semaphore_create(value);
  #else
    sem_init(&s->sem, 0, value);
  #endif
}

static inline void rk_sema_destroy(struct rk_sema *s)
{
  #ifdef __APPLE__
    //nothing
  #else
    sem_destroy(&s->sem);
  #endif
}

static inline void rk_sema_wait(struct rk_sema *s)
{

  #ifdef __APPLE__
    dispatch_semaphore_wait(s->sem, DISPATCH_TIME_FOREVER);
  #else
    int r;

    do {
            r = sem_wait(&s->sem);
    } while (r == -1 && errno == EINTR);
  #endif
}

static inline void rk_sema_post(struct rk_sema *s)
{

  #ifdef __APPLE__
    dispatch_semaphore_signal(s->sem);
  #else
    sem_post(&s->sem);
  #endif
}

static void* work_thread_loop(struct thread_pool *tp)
{
    struct queue_item *item = NULL;
    int loop = 0;
    while(1){
        //线程阻塞，等待信号
        rk_sema_wait(tp->sem);
        DEBUG(10, "------>=====thread:<%u> loop[%d]=====<--------\n", (unsigned int)pthread_self(), ++loop);

        pthread_mutex_lock(&tp->lock);
        item = pop_queue_item(tp->data_queue);
        pthread_mutex_unlock(&tp->lock);
        if(NULL != item){
            tp->handle(item->conn, item->packet);
            destroy_mqtt_packet(item->packet);
            destroy_queue_item(item);
        }
    }  
}

