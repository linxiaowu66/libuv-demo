/*
 *  启动Tcp服务器的顺序
 *  1、初始化uv_tcp_t: uv_tcp_init(loop, &tcp_server)
 *  2、绑定地址：uv_tcp_bind
 *  3、监听连接：uv_listen
 *  4、每当有一个连接进来之后，调用uv_listen的回调，回调里要做如下事情：
 *    4.1、初始化客户端的tcp句柄：uv_tcp_init()
 *    4.2、接收该客户端的连接：uv_accept()
 *    4.3、开始读取客户端请求的数据：uv_read_start()
 *    4.4、读取结束之后做对应操作，如果需要响应客户端数据，调用uv_write，回写数据即可。
 * 除了上述知识点，本demo还用到了timer句柄。
 */

#include <stdio.h>
#include "uv.h"
#include "common.h"


#define HOST "0.0.0.0"
#define PORT 9999

// 静态tcp句柄
static uv_tcp_t tcp_server_handle;

void close_cb(uv_handle_t *client) {
  // 释放这个tcp_client_handle
  free(client);
  printf("connection closed\n");
}

void shutdown_cb(uv_shutdown_t *req, int status) {
  // 关闭这个shutdown_req
  uv_close((uv_handle_t *)req->handle, close_cb);
  free(req);
}

void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
  buf->base = malloc(suggested_size);
  buf->len = suggested_size;
  if (buf->base == NULL) {
    printf("alloc_cb malloc buffer error\n");
  }
}

void write_cb(uv_write_t* req, int status) {
  CHECK(status, "write_cb");
  // 释放掉我们之前分配的uv_write_req

  printf("server had reponsed\n");

  write_req_t *write_req = (write_req_t *)req;

  // 这里不再需要特殊释放，因为这里的Buf不是malloc的
//  free(write_req->buf.base);
  free(write_req);
}

void write_to_client(char *resp, uv_stream_t* stream) {
  int r = 0;
  write_req_t * write_req = malloc(sizeof(uv_fs_t));
  write_req->buf = uv_buf_init(resp, sizeof(resp));
  r = uv_write(&write_req->req, stream, &write_req->buf, sizeof(resp), write_cb);
  CHECK(r, "uv_write");
}

void read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
  int r = 0;
  // 读数据包最重要的是判断nread这个字段
  if (nread < 0) {
    if (nread != UV_EOF) {
      CHECK(nread, "read_cb");
    }

    free(buf->base);

    // 读取数据到结尾了，客户端没有数据需要发送了
    uv_shutdown_t *shutdown_req = malloc(sizeof(uv_shutdown_t));
    r = uv_shutdown(shutdown_req, stream, shutdown_cb);
    CHECK(r, "uv_shutdown");
    return;
  }

  if (nread == 0) {
    free(buf->base);
    return;
  }

  // 正常读取数据，判断数据是不是我们想要的，不是的话就返回错误的消息告知客户端
  if (!strcmp("Hello\n", buf->base)) {
    write_to_client("world\n", stream);
  } else if (!strcmp("Libuv\n", buf->base)) {
    write_to_client("I love\n", stream);
  } else {
    write_to_client("Unknown argot\n", stream);
  }
}

void connection_cb(uv_stream_t *server, int status) {
  int r = 0;
  // 初始化客户端的tcp句柄
  uv_tcp_t *tcp_client_handle = malloc(sizeof(uv_tcp_t));
  r = uv_tcp_init(server->loop, tcp_client_handle);

  // 接受这个连接
  r = uv_accept(server, (uv_stream_t *)tcp_client_handle);

  printf("A client has connected to me\n");

  if (r < 0) {
    // 如果接受连接失败，需要清理一些东西
    uv_shutdown_t *shutdown_req = malloc(sizeof(uv_shutdown_t));

    r = uv_shutdown(shutdown_req, (uv_stream_t *)tcp_client_handle, shutdown_cb);
    CHECK(r, "uv_shutdown");
  }

  // 连接接受成功之后，开始读取客户端传输的数据
  r = uv_read_start((uv_stream_t *)tcp_client_handle, alloc_cb, read_cb);
}

void timer_cb(uv_timer_t *handle) {
  uv_print_active_handles(handle->loop, stderr);
}

int main() {
  uv_loop_t *loop = uv_default_loop();
  int r = 0;

  // 初始化tcp句柄，这里不会启动任何socket
  r = uv_tcp_init(loop, &tcp_server_handle);
  CHECK(r, "uv_tcp_init");


  // 初始化跨平台可用的ipv4地址
  struct sockaddr_in addr;
  r = uv_ip4_addr(HOST, PORT, &addr);
  CHECK(r, "uv_ipv4_addr");

  // 绑定
  r = uv_tcp_bind(&tcp_server_handle, (struct sockaddr *) &addr, AF_INET);
  CHECK(r, "uv_tcp_bind");

  // 开始监听连接
  r = uv_listen((uv_stream_t *)&tcp_server_handle, SOMAXCONN, connection_cb);
  CHECK(r, "uv_listen");

  // The stdout file handle (which printf writes to) is by default line buffered.
  // That means output is buffered until there is a newline, when the buffer is flushed.
  // That's why you should always end your output with a newline.
  // 所以如果你这里的printf打印后不加\n的话，所有的打印都会积攒在一起，直到有\n
  printf("tcp server listen at %s:%d\n", HOST, PORT);


  // 增加一个定时器去询问当前是不是一直有活跃的句柄，以此来验证某些观点
  uv_timer_t timer_handle;
  r = uv_timer_init(loop, &timer_handle);
  CHECK(r, "uv_timer_init");

  // 每10秒钟调用定时器回调一次
  r = uv_timer_start(&timer_handle, timer_cb, 10 * 1000, 10 * 1000);

  uv_run(loop, UV_RUN_DEFAULT);
}
