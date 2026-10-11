#include "../inc/threadpool.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
  threadpool_t pool;
  threadpool_init(&pool);

  for (int i = 0; i < 15; i++) {
    int* task_num = malloc(sizeof(int));
    *task_num = i;
    threadpool_add_task(&pool, example_task, task_num);
  }

  sleep(5);                 /* let tasks finish: destroy does not drain the queue */
  threadpool_destroy(&pool);
  return 0;
}
