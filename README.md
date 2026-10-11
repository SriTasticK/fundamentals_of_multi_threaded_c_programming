# Fundamentals of Multi-Threaded C Programming

A fixed-size POSIX thread pool implemented in C, built to learn the fundamentals of concurrent programming: mutexes, condition variables, work queues, and clean shutdown.

## Overview

A thread pool creates a fixed number of worker threads once and reuses them to run many tasks, avoiding the cost of creating and destroying a thread per task.

```
 submit task            +-----------------+        +-----------+
 ------------------->   |   task queue    | ---->  | worker 1  |
 submit task            |  (FIFO, locked) | ---->  | worker 2  |
 ------------------->   |                 | ---->  | worker N  |
                        +-----------------+        +-----------+
```

## Features

- Fixed number of worker threads, set at creation
- Thread-safe FIFO task queue (mutex + condition variable)
- Tasks are `void (*)(void *)` function pointers with a `void *` argument
- Blocking wait until all queued tasks finish
- Graceful shutdown: pending tasks complete, then workers exit and are joined

## Project Structure

```
.
├── include/
│   └── thread_pool.h     # public API
├── src/
│   ├── thread_pool.c     # pool implementation
│   └── main.c            # example / demo
├── tests/                # tests
├── Makefile
└── README.md
```

> Adjust the layout above to match the actual repository.

## Requirements

- GCC or Clang with C11 support
- POSIX threads (`pthread`) on Linux or macOS
- `make`

## Build

```bash
make            # build
make clean      # remove build artifacts
```

Without a Makefile:

```bash
gcc -std=c11 -Wall -Wextra -pthread -Iinclude src/*.c -o thread_pool_demo
```

## Usage

```c
#include "thread_pool.h"

static void work(void *arg) {
    int n = *(int *)arg;
    printf("task %d\n", n);
}

int main(void) {
    thread_pool_t *pool = thread_pool_create(4);   /* 4 workers */

    int ids[10];
    for (int i = 0; i < 10; i++) {
        ids[i] = i;
        thread_pool_submit(pool, work, &ids[i]);
    }

    thread_pool_wait(pool);       /* block until all tasks finish */
    thread_pool_destroy(pool);    /* stop and join workers, free memory */
    return 0;
}
```

## API

| Function | Description |
|---|---|
| `thread_pool_t *thread_pool_create(size_t num_threads)` | Create a pool with `num_threads` workers. Returns `NULL` on failure. |
| `int thread_pool_submit(thread_pool_t *pool, void (*fn)(void *), void *arg)` | Enqueue a task. Returns `0` on success, nonzero on failure. |
| `void thread_pool_wait(thread_pool_t *pool)` | Block until the queue is empty and all workers are idle. |
| `void thread_pool_destroy(thread_pool_t *pool)` | Finish pending tasks, join all workers, free resources. |

> Adjust names and signatures to match `thread_pool.h`.

## Design Notes

- **Synchronization:** one mutex guards the queue and pool state; a condition variable wakes idle workers when work arrives; a second condition signals when the pool becomes idle.
- **Shutdown:** a `shutdown` flag is set under the mutex, all workers are woken with `pthread_cond_broadcast`, and each worker exits once the queue is empty.
- **Lifetime of task arguments:** the pool does not copy `arg`. The caller must keep it valid until the task runs.

## Testing

```bash
make test
```

To check for data races and memory errors:

```bash
gcc -std=c11 -g -fsanitize=thread -pthread -Iinclude src/*.c -o tsan_build
gcc -std=c11 -g -fsanitize=address -pthread -Iinclude src/*.c -o asan_build
valgrind --tool=helgrind ./thread_pool_demo
```

## Known Limitations

- Fixed pool size (no dynamic resizing)
- Unbounded task queue
- No task priorities, cancellation, or return values

## Roadmap

- [ ] Bounded queue with backpressure
- [ ] Futures / task results
- [ ] Dynamic worker scaling
- [ ] Benchmarks against spawning a thread per task

## License

MIT (or choose your own and add a `LICENSE` file).
