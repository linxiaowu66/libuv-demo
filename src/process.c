/*
 * 利用libuv创建进程都是用的这个方法：uv_spawn，libuv利用执行另外一个文件程序来实现
 */
#include <stdio.h>
#include "uv.h"
#include "common.h"

int main() {
  uv_loop_t *loop = uv_default_loop();
  int r = 0;
  uv_process_t process_handle;

  r = uv_spawn(loop, &process_handle);

  uv_run(loop, UV_RUN_DEFAULT);
}
