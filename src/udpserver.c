/*
 *  启动udp服务器的顺序，因为udp是不建立连接，所以没有uv_listen
 *  1、初始化接收端的uv_udp_t: uv_udp_init(loop, &receive_socket_handle)
 *  2、绑定地址：uv_udp_bind
 *  3、开始接收消息：uv_udp_recv_start
 *  4、uv_udp_recv_start里执行回调，可以使用下面方法回写数据发送给客户端
 *    4.1、uv_udp_init初始化send_socket_handle
 *    4.2、uv_udp_bind绑定发送者的地址，地址可以从recv获取
 *    4.3、uv_udp_send发送指定消息
 *  除了上述知识点外，本demo还是用到signal句柄。
 */

#include <stdio.h>
#include "uv.h"
#include "common.h"


#define HOST "127.0.0.1"
#define PORT 9999

// receive套接字句柄
static uv_udp_t receive_socket_handle;

void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
  buf->base = malloc(suggested_size);
  buf->len = suggested_size;
  if (buf->base == NULL) {
    printf("alloc_cb malloc buffer error\n");
  }
}

void send_cb(uv_udp_send_t* req, int status) {
//  CHECK(status, "send_cb");
  printf("callback.......");
//  free(req);
}



void receive_cb(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned int flags) {
  int r = 0;
  // 读数据包最重要的是判断nread这个字段
  if (nread < 0) {
    // 因为udp不是使用stream形式，所以这里不需要使用uv_shutdown，直接调用uv_close
    fprintf(stderr, "recv error unexpected\n");
    uv_close((uv_handle_t *)handle, NULL);
    free(buf->base);
    return;
  }

  char sender[21] = { 0 };
  uv_ip4_name((struct sockaddr_in*) addr, sender, 20);
  fprintf(stderr, "recv from %s\n", sender);


  uv_udp_t send_socket_handle;
  uv_udp_send_t *send_req = malloc(sizeof(send_req));
  uv_buf_t newBuf = uv_buf_init(buf->base, nread);
  struct sockaddr_in client_addr;
  r = uv_ip4_addr(sender, 60581, &client_addr);
  // 反向发送消息给客户端
//   r = uv_udp_init(req->loop, &send_socket_handle);
//   CHECK(r, "uv_udp_init");
//   r = uv_udp_bind(&client_addr, addr, 0);
//   CHECK(r, "uv_udp_bind");

   r = uv_udp_send(send_req, &send_socket_handle, &newBuf, nread, addr, send_cb);
  CHECK(r, "uv_udp_send");
  // r = uv_udp_recv_stop(handle)
}

void timer_cb(uv_timer_t *handle) {
  uv_print_active_handles(handle->loop, stderr);
}
void signal_cb(uv_signal_t *handle, int signum) {
  printf("signal_cb: recvd CTRL+C shutting down\n");
  uv_stop(uv_default_loop()); //stops the event loop
}

int main() {
  uv_loop_t *loop = uv_default_loop();
  int r = 0;

  // 初始化udp句柄
  r = uv_udp_init(loop, &receive_socket_handle);
  CHECK(r, "uv_udp_init");


  // 初始化跨平台可用的ipv4地址
  struct sockaddr_in addr;
  r = uv_ip4_addr(HOST, PORT, &addr);
  CHECK(r, "uv_ipv4_addr");

  // 绑定
  r = uv_udp_bind(&receive_socket_handle, (const struct sockaddr *) &addr, 0);
  CHECK(r, "uv_udp_bind");

  // 开始接收消息
  r = uv_udp_recv_start(&receive_socket_handle, alloc_cb, receive_cb);
  CHECK(r, "uv_udp_recv_start");

  printf("udp server listen at %s:%d\n", HOST, PORT);


  // 增加一个定时器去询问当前是不是一直有活跃的句柄，以此来验证某些观点
  uv_timer_t timer_handle;
  r = uv_timer_init(loop, &timer_handle);
  CHECK(r, "uv_timer_init");

  // 每10秒钟调用定时器回调一次
  r = uv_timer_start(&timer_handle, timer_cb, 10 * 1000, 0);

  // 增加signal的监听
  uv_signal_t signal_handle;
  r = uv_signal_init(loop, &signal_handle);
  CHECK(r, "uv_signal_init");

  r = uv_signal_start(&signal_handle, signal_cb, SIGINT);

  uv_run(loop, UV_RUN_DEFAULT);
}
