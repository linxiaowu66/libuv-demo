/*
 * 在libuv中涉及到线程的总共有三部分：
 * 1、从线程池中调度一个线程运行回调: uv_queue_work
 * 2、使用async Handle来“唤醒” event loop并在别的线程上执行回调：uv_async_send
 * 3、使用uv_thread_create手动创建一个线程来执行
 * 本demo除了可以学到上面说的三种方式，还涉及到的知识点有：
 * 1、线程间通信
 * 2、线程池调度
 * 3、线程间读写数据同步原语
 */
#include <stdio.h>
#include "uv.h"
#include "common.h"

int shareMemory = 0;
uv_rwlock_t numlock;
uv_barrier_t barrier;
uv_async_t async_handle;
char msg[50];

// work_cb是会从线程池中调度一个线程去执行
void work_cb(uv_work_t *req) {
  printf("I am work callback, calling in some thread in thread pool, pid=>%d\n", uv_os_getpid());
//  uv_thread_t thread_handle = uv_thread_self();
  printf("work_cb thread id 0x%lx\n", (unsigned long int) uv_thread_self());

  // 发送消息给eventloop线程
  int r = 0;
  sprintf(msg, "This msg from another thread: 0x%lx", (unsigned long int) uv_thread_self());
  async_handle.data = (void *) &msg;
  r = uv_async_send(&async_handle);
  CHECK(r, "uv_async_send");
}

// after_work_cb是在event loop线程中执行
void after_work_cb(uv_work_t *req, int status) {
  printf("I am after work callback, calling from event loop thread, pid=>%d\n", uv_os_getpid());
  printf("after_work_cb thread id 0x%lx\n", (unsigned long int) uv_thread_self());
}

void async_cb(uv_async_t *handle) {
  printf("I am async callback, calling from event loop thread, pid=>%d\n", uv_os_getpid());
  printf("async_cb thread id 0x%lx\n", (unsigned long int) uv_thread_self());

  char *msg = (char *)handle->data;

  printf("I am receiving msg: %s\n", msg);

  // 关闭掉async句柄，让进程退出
//  uv_close((uv_handle_t *)&async_handle, NULL);
}

void timer_cb(uv_timer_t *handle) {
  uv_print_active_handles(handle->loop, stderr);
}

// 我们使用读写锁来演示线程间读写公共内存的一种保护机制
void reader(void *args) {
  unsigned long int threadId = (unsigned long int)uv_thread_self();
  printf("I am the reader thread, threadId => 0x%lx\n", threadId);

  thread_args_t threadArgs = *(thread_args_t *)args;

  printf("[0x%lx-reader/%d]master process pass the args: %s\n", threadId, threadArgs.number, threadArgs.string);

  int i = 0;

  for(;i < 2; i++) {
    uv_rwlock_rdlock(&numlock);
    printf("[0x%lx-reader/%d] read the share memory: %d\n", threadId, threadArgs.number, shareMemory);
    uv_rwlock_rdunlock(&numlock);
    printf("[0x%lx-reader/%d] release the rwlock\n", threadId, threadArgs.number);
  }

  printf("[0x%lx-reader/%d]will be exist\n", threadId, threadArgs.number);
  // 等待该线程结束
  uv_barrier_wait(&barrier);
}

void writer(void *args) {
  unsigned long int threadId = (unsigned long int)uv_thread_self();
  printf("I am the writer thread, threadId => 0x%lx\n", threadId);

  thread_args_t threadArgs = *(thread_args_t *)args;

  printf("[0x%lx-writer/%d]master process pass the args: %s\n", threadId, threadArgs.number, threadArgs.string);

  int i = 0;

  for(;i < 2; i++) {
    uv_rwlock_wrlock(&numlock);
    printf("[0x%lx-writer/%d] write the share memory: %d\n", threadId, threadArgs.number, shareMemory);
    shareMemory++;
    uv_rwlock_wrunlock(&numlock);
    printf("[0x%lx-writer/%d] release the rwlock\n", threadId, threadArgs.number);
  }

  printf("[0x%lx-writer/%d]will be exist\n", threadId, threadArgs.number);
  // 等待该线程结束
  uv_barrier_wait(&barrier);
}

int main() {
  uv_loop_t *loop = uv_default_loop();
  int r = 0;
  uv_rwlock_init(&numlock);
  // 这里是4个线程，包含读线程2个、写线程1个、以及event loop线程
  uv_barrier_init(&barrier, 4);

  printf("I am the master process, processId => %d\n", uv_os_getpid());

  // 首先示例uv_queue_wok的用法
  uv_work_t work_handle;
  r = uv_queue_work(loop, &work_handle, work_cb, after_work_cb);
  CHECK(r, "uv_queue_work");

  // 接着测试async_handle的用法,初始化这个方法之后，进程不会主动退出，只有close掉才会
  r = uv_async_init(loop, &async_handle, async_cb);
  CHECK(r, "uv_async_init");


  // 第三种是自己手动创建线程：uv_thread_create
  uv_thread_t thread_handle[3];
  thread_args_t thread_args[3] = {{
    .string = "I am reader thread one",
    .number = 0,
  }, {
    .string = "I am reader thread two",
    .number = 1,
  }, {
    .string = "I am writer thread one",
    .number = 0,
  }};
  r = uv_thread_create(&thread_handle[0], reader, &thread_args[0]);
  CHECK(r, "uv_thread_create");
  r = uv_thread_create(&thread_handle[1], reader, &thread_args[1]);
  CHECK(r, "uv_thread_create");
  r = uv_thread_create(&thread_handle[2], writer, &thread_args[2]);
  CHECK(r, "uv_thread_create");
  // 该函数是让event loop等待我创建的线程执行完再退出
  // r = uv_thread_join(&thread_handle);
  // CHECK(r, "uv_thread_join");
  // 但是因为现在我使用了async handle，event loop这个线程不会退出去，所以不再需要使用
  // 除了使用thread_join之外，还可以使用uv_barrier_wait来实现这个过程

  uv_barrier_wait(&barrier);
  printf("I am event loop thread => 0x%lx\n", (unsigned long int)uv_thread_self());

 // 为什么我这里的uv_barrier_wait失败了呢？
  uv_barrier_destroy(&barrier);



  // 增加一个定时器去询问当前是不是一直有活跃的句柄，以此来验证某些观点
  uv_timer_t timer_handle;
  r = uv_timer_init(loop, &timer_handle);
  CHECK(r, "uv_timer_init");

  // 10秒钟后调用定时器回调一次
  r = uv_timer_start(&timer_handle, timer_cb, 10 * 1000, 0);

  // 这里destroy读写锁的话，如果不等上面的线程全部执行完，进程会报错，process exist with code 6。
  uv_rwlock_destroy(&numlock);

  return uv_run(loop, UV_RUN_DEFAULT);
}
