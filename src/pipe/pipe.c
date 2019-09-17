/*
 * 使用pipe句柄的步骤：
 * 1、初始化管道：uv_pipe_init
 * 2、打开管道：uv_pipe_open
 * 使用管道你要想象出需要有管道输入源和输出方，比如讲标准输入作为管道的输入方，文件作为输出方，这种模式都是可行的。
 * 下面的例子我们将tcp流作为管道的输入方，然后将该信息流输出到随机的一个进程中的标准输出以观察该模型。
 */
#include <stdio.h>
#include "uv.h"
#include "../common.h"

#define STDIN   0
#define STDOUT  1
#define STDERR  2

#define NOIPC 0
#define IPC   1

uv_loop_t *loop;

struct child_worker {
  uv_process_t req;
  uv_process_options_t options;
  uv_pipe_t pipe;
} *workers;

int round_robin_counter;
int child_worker_count;

char exepath[PATH_MAX];

uv_buf_t dummy_buf;

void alloc_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
  buf->base = malloc(size);
  buf->len = size;
}

void on_exit(uv_process_t* req, int64_t exit_status, int term_signal) {
  fprintf(stderr, "Process exited with status %lld, signal %d\n", exit_status, term_signal);
  uv_close((uv_handle_t*) req, NULL);
}

void connection_cb(uv_stream_t *server, int status) {
  int r;
  if (status) {
    fprintf(stderr, "connection error %d", status);
    return;
  }

  // main diff between uv_tcp_t and uv_pipe_t is the ipc flag and the pipe_fname pointing to local sock file
  uv_pipe_t *client = (uv_pipe_t*) malloc(sizeof(uv_pipe_t));
  uv_pipe_init(loop, client, NOIPC);
  r = uv_accept(server, (uv_stream_t*) client);

  if (r == 0) {
    uv_write_t *write_req = (uv_write_t*) malloc(sizeof(uv_write_t));
    dummy_buf = uv_buf_init(".", 1);

    struct child_worker *worker = &workers[round_robin_counter];

    uv_write2(write_req, (uv_stream_t*) &worker->pipe, &dummy_buf, 1 /*nbufs*/, (uv_stream_t*) client, NULL);

    round_robin_counter = (round_robin_counter + 1) % child_worker_count;
  } else {
    uv_close((uv_handle_t*) client, NULL);
  }
}

char* exepath_for_worker() {
  int r;
  size_t size = PATH_MAX;

  r = uv_exepath(exepath, &size);
  CHECK(r, "getting executable path");
  strcpy(exepath + (strlen(exepath) - strlen("PipeHandle")), "WorkerHandle");
  return exepath;
}

int get_cpu_count() {
  int r;
  uv_cpu_info_t *info;
  int cpu_count;
  r = uv_cpu_info(&info, &cpu_count);
  CHECK(r, "obtaining cpu info");

  uv_free_cpu_info(info, cpu_count);
  return cpu_count;
}

void setup_workers() {
  int r;
  exepath_for_worker();

  char* args[2];
  args[0] = exepath;
  args[1] = NULL;

  round_robin_counter = 0;
  int cpu_count = get_cpu_count();
  child_worker_count = cpu_count;

  workers = calloc(sizeof(struct child_worker), cpu_count);
  while (cpu_count--) {
    struct child_worker *worker = &workers[cpu_count];
    // pipe is acting as IPC channel
    uv_pipe_init(loop, &worker->pipe, IPC);

    uv_stdio_container_t child_stdio[3];
    child_stdio[STDIN].flags       =  UV_CREATE_PIPE | UV_READABLE_PIPE;
    child_stdio[STDIN].data.stream =  (uv_stream_t*) &worker->pipe;
    child_stdio[STDOUT].flags      =  UV_IGNORE;
    child_stdio[STDERR].flags      =  UV_INHERIT_FD;
    child_stdio[STDERR].data.fd    =  STDERR;

    worker->options.stdio_count =  3;
    worker->options.stdio       =  child_stdio;
    worker->options.exit_cb     =  on_exit;
    worker->options.file        =  exepath;
    worker->options.args        =  args;

    r = uv_spawn(loop, &worker->req, &worker->options);
    CHECK(r, "spawning worker");

    fprintf(stderr, "Started worker %d\n", worker->req.pid);
  }
}

int main() {
  int r;
  loop = uv_default_loop();

  setup_workers();

  struct sockaddr_in bind_addr;
  r = uv_ip4_addr("0.0.0.0", 7000, &bind_addr);
  CHECK(r, "obtaining ipv4 address");

  uv_tcp_t server;
  uv_tcp_init(loop, &server);
  r = uv_tcp_bind(&server, (const struct sockaddr*) &bind_addr, 0);
  CHECK(r, "binding server to socket");

  r = uv_listen((uv_stream_t*) &server, 128/*backlog*/, connection_cb);
  CHECK(r, "listening for connections");

  return uv_run(loop, UV_RUN_DEFAULT);
}
