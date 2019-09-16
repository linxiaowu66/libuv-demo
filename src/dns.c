/*
 * 使用libuv进行DNS查询
 * 使用uv_getaddrinfo查询域名的对应的IP地址
 * 使用uv_getnameinfo查询IP地址对应的域名              
 */
#include <stdio.h>
#include "uv.h"
#include "common.h"

void name_cb(uv_getnameinfo_t* req, int status, const char* hostname, const char* service) {
  CHECK(status, "name_cb");

  printf("resolve the host name: %s\n", req->host);
//  printf("resolve the service: %s\n", req->service);
}

void addr_cb(uv_getaddrinfo_t *req, int status, struct addrinfo *res) {
  CHECK(status, "addr_cb");

  char addr[17] = {"\0"};

  uv_ip4_name((struct sockaddr_in *) res->ai_addr, addr, 16);

  printf("resolve ip: %s\n", addr);

  uv_getnameinfo_t nameinfo_req;
  int r = 0;

//  struct sockaddr_in host;
//  uv_ip4_addr("47.96.8.45", 80, &host);

  r = uv_getnameinfo(req->loop, &nameinfo_req, name_cb, res->ai_addr, 0);
  CHECK(r, "uv_getnameinfo");

  uv_freeaddrinfo(res);
}

int main() {
  uv_loop_t *loop = uv_default_loop();

  struct addrinfo hints;

  // 这些宏定义都是些什么意思？AF(Address Family)、PF(Protocol Family), http://man7.org/linux/man-pages/man2/socket.2.html
  hints.ai_family = PF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  int r = 0;
  uv_getaddrinfo_t addrinfo_req;
  r = uv_getaddrinfo(loop, &addrinfo_req, addr_cb, "blog.5udou.cn", NULL, &hints);

  CHECK(r, "uv_getaddrinfo");

  uv_run(loop, UV_RUN_DEFAULT);
}
