#include <stdio.h>
#include "uv.h"

void idle_cb(uv_idle_t *handle) {
  printf("I am the idle handle callback");
  uv_idle_stop(handle);
}

// 该文件展示idle handle的使用, 记住，idle handle不能听名字就觉得是event loop在空闲的时候才调用的，
// 它的调用顺序是有规定的
int main() {
  uv_loop_t *loop = uv_default_loop();

  uv_idle_t idle_handle;
  uv_idle_init(loop, &idle_handle);
  uv_idle_start(&idle_handle, idle_cb);

  printf("I will run firstly\n");
  uv_run(loop, UV_RUN_DEFAULT);
}
