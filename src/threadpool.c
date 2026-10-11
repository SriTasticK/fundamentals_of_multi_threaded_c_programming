#include "../inc/threadpool.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

/* Registry of live pools: calls on never-initialized or destroyed pools are ignored. */
#define MAX_POOLS 64
static pthread_mutex_t registry_lock = PTHREAD_MUTEX_INITIALIZER;
static threadpool_t* registry[MAX_POOLS];

static void registry_add(threadpool_t* p) {      /* idempotent */
  pthread_mutex_lock(&registry_lock);
  for (int i = 0; i < MAX_POOLS; i++)
    if (registry[i] == p) { pthread_mutex_unlock(&registry_lock); return; }
  for (int i = 0; i < MAX_POOLS; i++)
    if (registry[i] == NULL) { registry[i] = p; break; }
  pthread_mutex_unlock(&registry_lock);
}

static int registry_has(threadpool_t* p) {
  int found = 0;
  pthread_mutex_lock(&registry_lock);
  for (int i = 0; i < MAX_POOLS; i++)
    if (registry[i] == p) { found = 1; break; }
  pthread_mutex_unlock(&registry_lock);
  return found;
}

static void registry_remove(threadpool_t* p) {
  pthread_mutex_lock(&registry_lock);
  for (int i = 0; i < MAX_POOLS; i++)
    if (registry[i] == p) registry[i] = NULL;
  pthread_mutex_unlock(&registry_lock);
}

static void* thread_function(void* arg) {
  threadpool_t* pool = arg;

  for (;;) {
    pthread_mutex_lock(&pool->lock);

    while (pool->queued == 0 && !pool->stop)
      pthread_cond_wait(&pool->notify, &pool->lock);

    if (pool->stop) {                       /* matches reference: exit without draining */
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
  registry_add(pool);

  for (size_t i = 0; i < THREADS; i++) {
    if (pthread_create(&pool->threads[i], NULL, thread_function, pool) != 0)
      pool->threads[i] = 0;
  }
}

void threadpool_add_task(threadpool_t* pool, void (*function)(void*), void* arg) {
  if (pool == NULL || function == NULL || !registry_has(pool)) return;

  pthread_mutex_lock(&pool->lock);

  if (!pool->stop && pool->queued >= 0 && pool->queued < QUEUE_SIZE) {
    pool->task_queue[pool->queue_back].fn = function;
    pool->task_queue[pool->queue_back].arg = arg;
    pool->queue_back = (pool->queue_back + 1) % QUEUE_SIZE;
    pool->queued++;
    pthread_cond_signal(&pool->notify);
  } else {
    printf("Task queue is full! Cannot add more tasks.\n");
  }

  pthread_mutex_unlock(&pool->lock);
}

void threadpool_destroy(threadpool_t* pool) {
  if (pool == NULL || !registry_has(pool)) return;

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

  registry_remove(pool);
  pthread_mutex_destroy(&pool->lock);
  pthread_cond_destroy(&pool->notify);
}

void example_task(void* arg) {
  int* num = (int*)arg;
  if (num == NULL) return;
  printf("Processing task %d\n", *num);
  sleep(1);
  free(arg);
}
