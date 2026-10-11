#include "threadpool.h"
#include <stdio.h>
#include <unistd.h>

static void example_task(void* arg) {
  int id = *(int*)arg;
  printf("task %d on thread %lu\n", id, (unsigned long)pthread_self());
  usleep(100000);
}

int main(void) {
  threadpool_t pool;
  int ids[50];

  threadpool_init(&pool);

  for (int i = 0; i < 50; i++) {
    ids[i] = i;
    if (threadpool_add_task(&pool, example_task, &ids[i]) != 0)
      fprintf(stderr, "task %d rejected\n", i);
  }

  threadpool_destroy(&pool);
  puts("done");
  return 0;
}
