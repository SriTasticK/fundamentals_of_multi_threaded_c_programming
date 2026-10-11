#include "../inc/threadpool.h"
#include <stddef.h>

static void* thread_function(void* arg) {
  threadpool_t* pool = arg;

  for (;;) {
    pthread_mutex_lock(&pool->lock);

    while (pool->queued == 0 && !pool->stop)
      pthread_cond_wait(&pool->notify, &pool->lock);

    if (pool->queued == 0) {            /* stop set and queue drained */
      pthread_mutex_unlock(&pool->lock);
      return NULL;
    }

    task_t task = pool->task_queue[pool->queue_front];
    pool->queue_front = (pool->queue_front + 1) % QUEUE_SIZE;
    pool->queued--;

    pthread_cond_broadcast(&pool->notify);   /* a slot is free: wake blocked producers */
    pthread_mutex_unlock(&pool->lock);

    task.fn(task.arg);
  }
}

void threadpool_init(threadpool_t* pool) {
  if (pool == NULL) return;

  pool->queued = 0;
  pool->queue_front = 0;
  pool->queue_back = 0;
  pool->stop = 0;

  pthread_mutex_init(&pool->lock, NULL);
  pthread_cond_init(&pool->notify, NULL);

  for (size_t i = 0; i < THREADS; i++)
    pthread_create(&pool->threads[i], NULL, thread_function, pool);
}

void threadpool_add_task(threadpool_t* pool, void (*function)(void*), void* arg) {
  if (pool == NULL || function == NULL) return;

  pthread_mutex_lock(&pool->lock);

  while (pool->queued == QUEUE_SIZE && !pool->stop)
    pthread_cond_wait(&pool->notify, &pool->lock);   /* block until space */

  if (pool->stop) {                                  /* pool shutting down: drop */
    pthread_mutex_unlock(&pool->lock);
    return;
  }

  pool->task_queue[pool->queue_back].fn = function;
  pool->task_queue[pool->queue_back].arg = arg;
  pool->queue_back = (pool->queue_back + 1) % QUEUE_SIZE;
  pool->queued++;

  pthread_cond_broadcast(&pool->notify);   /* broadcast: waiters may be producers or workers */
  pthread_mutex_unlock(&pool->lock);
}

void threadpool_destroy(threadpool_t* pool) {
  if (pool == NULL) return;

  pthread_mutex_lock(&pool->lock);
  pool->stop = 1;
  pthread_cond_broadcast(&pool->notify);
  pthread_mutex_unlock(&pool->lock);

  for (size_t i = 0; i < THREADS; i++)
    pthread_join(pool->threads[i], NULL);

  pthread_mutex_destroy(&pool->lock);
  pthread_cond_destroy(&pool->notify);
}

/* Weak: a test that defines its own example_task overrides this one. */
__attribute__((weak)) void example_task(void* arg) {
  (void)arg;
}
