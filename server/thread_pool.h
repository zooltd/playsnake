#ifndef SERVER_THREAD_POOL_H

#include <pthread.h>

#include <pthread.h>
#include <ctype.h>

typedef struct tpool_work {
    void *(*work_routine)(void *); //function to be called
    void *args;                   //arguments
    struct tool_work *next;
} tpool_work_t;

typedef struct tpool {
    size_t shutdown;       //is tpool shutdown or not, 1 ---> yes; 0 ---> no
    size_t maxnum_thread; // maximum of threads
    pthread_t *thread_id;     // a array of threads
    tpool_work_t *tpool_head;     // tpool_work queue
    pthread_cond_t queue_ready;    // condition varaible
    pthread_mutex_t queue_lock;     // queue lock
} tpool_t;

/**
 * 创建线程池
 * @param pool
 * @param max_thread_num
 * @return 0: success | other: fail
 */
int create_tpool(tpool_t **pool, size_t max_thread_num);

/**
 * destroy thread pool
 * @param pool
 */
void destroy_tpool(tpool_t *pool);

/**
 *  add tasks to thread pool
 * @param pool
 * @param routine
 * @param args
 * @return
 */
int add_task_2_tpool(tpool_t *pool, void *(*routine)(void *), void *args);

#define SERVER_THREAD_POOL_H

#endif //SERVER_THREAD_POOL_H
