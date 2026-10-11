#include "../inc/threadpool.h"
#include <stdio.h>
#include <unistd.h>

static void work(void* arg) {
  printf("task %d\n", *(int*)arg);
  usleep(100000);
}

int main(void) {
  threadpool_t pool;
  int ids[200];

  threadpool_init(&pool);
  for (int i = 0; i < 200; i++) {
    ids[i] = i;
    threadpool_add_task(&pool, work, &ids[i]);   /* blocks when the queue is full */
  }
  threadpool_destroy(&pool);
  puts("done");
  return 0;
}
