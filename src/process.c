/*
 * 利用libuv创建进程都是用的这个方法：uv_spawn，libuv通过起另外一个进程去执行对应的文件
 * 子进程的打印信息通过配置参数可以打印出来的
 * https://stackoverflow.com/questions/14751504/capture-a-child-processs-stdout-with-libuv
 */
#include <stdio.h>
#include "uv.h"
#include "common.h"

char exepath[PATH_MAX];

#define STDIN   0
#define STDOUT  1
#define STDERR  2

void on_exit(uv_process_t* child_process, int64_t exit_status, int term_signal) {
  printf("Process exited with status %lld, signal %d\n", exit_status, term_signal);
  uv_close((uv_handle_t*) child_process, NULL);
}

char* exepath_for_process() {
  int r;
  size_t size = PATH_MAX;

  r = uv_exepath(exepath, &size);
  CHECK(r, "getting executable path");
  strcpy(exepath + (strlen(exepath) - strlen("ProcessHandle")), "FsHandle");
  return exepath;
}

int main() {
  uv_loop_t *loop = uv_default_loop();
  int r = 0;
  uv_process_t process_handle;
  uv_process_options_t options;

  const char* exepath = exepath_for_process();
  char *args[3] = { (char*) exepath, NULL, NULL };

  options.exit_cb = on_exit;
  options.file = exepath;
  options.args = args;
  options.stdio_count = 3;
  uv_stdio_container_t child_stdio[3];


  child_stdio[STDIN].flags       =  UV_IGNORE;
  child_stdio[STDOUT].flags      =  UV_INHERIT_FD;
  child_stdio[STDOUT].data.fd   =  STDOUT;
  child_stdio[STDERR].flags      =  UV_INHERIT_FD;
  child_stdio[STDERR].data.fd    =  STDERR;


  options.stdio = child_stdio;

  r = uv_spawn(loop, &process_handle, &options);
  printf("I am parent process, processId => %d, my child processId => %d\n", uv_os_getpid(), process_handle.pid);
  CHECK(r, "uv_spawn");

  uv_run(loop, UV_RUN_DEFAULT);
}
