#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "uv.h"
#include "../common.h"

#define STDIN   0
#define STDOUT  1
#define STDERR  2

#define NOIPC 0
#define IPC   1

uv_loop_t *loop;
uv_pipe_t queue;

void alloc_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
  buf->base = malloc(size);
  buf->len = size;
}

void write_cb(uv_write_t* req, int status) {
  CHECK(status, "async write");
  char *base = (char*) req->data;
  free(base);
  free(req);
}

void read_cb(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf) {
  if (nread < 0) {
    fprintf(stderr, "read error: [%s: %s]\n", uv_err_name((nread)), uv_strerror((nread)));
    uv_close((uv_handle_t*) client, NULL);
    return;
  }

  uv_write_t *req = (uv_write_t*) malloc(sizeof(uv_write_t));

  int len = strlen(buf->base);
  char tmp[len];
  strncpy(tmp, buf->base, len);
  sprintf(buf->base, "From worker %d => %s", getpid(), tmp);

  req->data = (void*) buf->base;

  uv_write(req, (uv_stream_t*)client, buf, 1, write_cb);
}

void on_new_connection(uv_stream_t *q, ssize_t nread, const uv_buf_t *buf) {
  if (nread < 0) {
    if (nread != UV_EOF)
      fprintf(stderr, "Read error %s\n", uv_err_name(nread));
    uv_close((uv_handle_t*) q, NULL);
    return;
  }

  uv_pipe_t *pipe = (uv_pipe_t*) q;
  if (!uv_pipe_pending_count(pipe)) {
    fprintf(stderr, "No pending count\n");
    return;
  }

  uv_handle_type pending = uv_pipe_pending_type(pipe);
  assert(pending == UV_TCP);

  uv_tcp_t *client = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
  uv_tcp_init(loop, client);
  if (uv_accept(q, (uv_stream_t*) client) == 0) {
    uv_os_fd_t fd;
    uv_fileno((const uv_handle_t*) client, &fd);
    fprintf(stderr, "Worker %d: Accepted fd %d\n", getpid(), fd);
    uv_read_start((uv_stream_t*) client, alloc_cb, read_cb);
  }
  else {
    uv_close((uv_handle_t*) client, NULL);
  }
}

int main(void) {
  loop = uv_default_loop();
  int r = 0;

  uv_pipe_init(loop, &queue, IPC);
  uv_pipe_open(&queue, STDIN);

  r = uv_read_start((uv_stream_t*)&queue, alloc_cb, on_new_connection);
  CHECK(r, "uv_read_start");

  return uv_run(loop, UV_RUN_DEFAULT);
}
