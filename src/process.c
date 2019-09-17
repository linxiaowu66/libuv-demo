/*
 * 利用libuv创建进程都是用的这个方法：uv_spawn，libuv通过起另外一个进程去执行对应的文件
 * 这里仍没明白的是，子进程打印的信息全都没有在父进程的stdio中打印出来(printf不行，fprintf(stderr)好像可以)。
 * https://stackoverflow.com/questions/14751504/capture-a-child-processs-stdout-with-libuv
 */
#include <stdio.h>
#include "uv.h"
#include "common.h"

void on_exit(uv_process_t* child_process, int64_t exit_status, int term_signal) {
  printf("Process exited with status %lld, signal %d\n", exit_status, term_signal);
  uv_close((uv_handle_t*) child_process, NULL);
}

int main() {
  uv_loop_t *loop = uv_default_loop();
  int r = 0;
  uv_process_t process_handle;
  uv_process_options_t options;

  const char* exepath = "pwd";
  char *args[3] = { (char*) exepath, NULL, NULL };

  options.exit_cb = on_exit;
  options.file = exepath;
  options.args = args;
  // options.stdio_count = 3;
  // uv_stdio_container_t child_stdio[3];
  // child_stdio[1].flags = UV_INHERIT_FD | UV_INHERIT_STREAM;
  // options.stdio = child_stdio;

  r = uv_spawn(loop, &process_handle, &options);
  printf("I am parent process, processId => %d, my child processId => %d\n", uv_os_getpid(), process_handle.pid);
  CHECK(r, "uv_spawn");

  uv_run(loop, UV_RUN_DEFAULT);
}
