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

// work_cb是会从线程池中调度一个线程去执行
void work_cb(uv_work_t *req) {
  printf("I am work callback, calling in some thread in thread pool, pid=>%d\n", uv_os_getpid());
//  uv_thread_t thread_handle = uv_thread_self();
  printf("work_cb thread id 0x%lx\n", (unsigned long int) uv_thread_self());
}

// after_work_cb是在event loop线程中执行
void after_work_cb(uv_work_t *req, int status) {
  printf("I am after work callback, calling from event loop thread, pid=>%d\n", uv_os_getpid());
  printf("after_work_cb thread id 0x%lx\n", (unsigned long int) uv_thread_self());
}

void async_cb(uv_async_t *handle) {
  printf("I am async callback, calling from event loop thread, pid=>%d\n", uv_os_getpid());
  printf("async_cb thread id 0x%lx\n", (unsigned long int) uv_thread_self());
}

void timer_cb(uv_timer_t *handle) {
  uv_print_active_handles(handle->loop, stderr);
}

int main() {
  uv_loop_t *loop = uv_default_loop();
  int r = 0;

  printf("I am the master process, processId => %d\n", uv_os_getpid());

  // 首先示例uv_queue_wok的用法
  uv_work_t work_handle;
  r = uv_queue_work(loop, &work_handle, work_cb, after_work_cb);
  CHECK(r, "uv_queue_work");

  // 接着测试async_handle的用法,初始化这个方法之后，进程不会主动退出，只有close掉才会
  uv_async_t async_handle;
  r = uv_async_init(loop, &async_handle, async_cb);
  CHECK(r, "uv_async_init");

  r = uv_async_send(&async_handle);
  CHECK(r, "uv_async_send");

  // 增加一个定时器去询问当前是不是一直有活跃的句柄，以此来验证某些观点
  uv_timer_t timer_handle;
  r = uv_timer_init(loop, &timer_handle);
  CHECK(r, "uv_timer_init");

  // 10秒钟后调用定时器回调一次
  r = uv_timer_start(&timer_handle, timer_cb, 10 * 1000, 0);

  return uv_run(loop, UV_RUN_DEFAULT);
}
