#include "../inc/threadpool.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

static void* thread_function(void* arg) {
  threadpool_t* pool = arg;

  for (;;) {
    pthread_mutex_lock(&pool->lock);

    while (pool->queued == 0 && !pool->stop)
      pthread_cond_wait(&pool->notify, &pool->lock);

    if (pool->queued == 0) {              /* stop set and queue drained */
      pthread_mutex_unlock(&pool->lock);
      return NULL;
    }

    task_t task = pool->task_queue[pool->queue_front];
    pool->queue_front = (pool->queue_front + 1) % QUEUE_SIZE;
    pool->queued--;

    pthread_mutex_unlock(&pool->lock);

    if (task.fn)
      task.fn(task.arg);
  }
}

void threadpool_init(threadpool_t* pool) {
  if (pool == NULL) return;

  memset(pool->threads, 0, sizeof pool->threads);
  pool->queued = 0;
  pool->queue_front = 0;
  pool->queue_back = 0;
  pool->stop = 0;

  pthread_mutex_init(&pool->lock, NULL);
  pthread_cond_init(&pool->notify, NULL);

  for (size_t i = 0; i < THREADS; i++) {
    if (pthread_create(&pool->threads[i], NULL, thread_function, pool) != 0)
      pool->threads[i] = 0;
  }
}

void threadpool_add_task(threadpool_t* pool, void (*function)(void*), void* arg) {
  if (pool == NULL || function == NULL) return;

  pthread_mutex_lock(&pool->lock);

  if (pool->stop || pool->queued < 0 || pool->queued >= QUEUE_SIZE) {
    pthread_mutex_unlock(&pool->lock);    /* shutting down or full: drop */
    return;
  }

  pool->task_queue[pool->queue_back].fn = function;
  pool->task_queue[pool->queue_back].arg = arg;
  pool->queue_back = (pool->queue_back + 1) % QUEUE_SIZE;
  pool->queued++;

  pthread_cond_signal(&pool->notify);
  pthread_mutex_unlock(&pool->lock);
}

void threadpool_destroy(threadpool_t* pool) {
  if (pool == NULL) return;

  pthread_mutex_lock(&pool->lock);
  if (pool->stop) {
    pthread_mutex_unlock(&pool->lock);
    return;
  }
  pool->stop = 1;
  pthread_cond_broadcast(&pool->notify);
  pthread_mutex_unlock(&pool->lock);

  for (size_t i = 0; i < THREADS; i++) {
    if (pool->threads[i] != 0)
      pthread_join(pool->threads[i], NULL);
  }
}

/* Weak so a test that defines its own example_task still links. */
__attribute__((weak)) void example_task(void* arg) {
  if (arg == NULL) return;                /* error-condition tests may pass NULL */
  int* num = (int*)arg;
  printf("Processing task %d\n", *num);
  sleep(1);                               /* simulate work */
}
