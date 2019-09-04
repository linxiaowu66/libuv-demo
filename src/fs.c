#include <stdio.h>
#include "uv.h"
#include "common.h"

static const char *filename = "./test1.txt";

void open_cb(uv_fs_t* open_req) {
  printf("open successfully：%d\n", open_req->result);
}

int main() {
  uv_loop_t *loop = uv_default_loop();

  uv_fs_t *open_req = malloc(sizeof(uv_fs_t));

  uv_context_t *context = malloc(sizeof(uv_context_t));

  // 分配一块buf存储读取的数据
  size_t buf_size = sizeof(char) * 1024;
  char *buf = malloc(buf_size);

  // 将上述两块内存的指针赋值给context，以便后面使用
  context->open_req = open_req;
  context->buf = uv_buf_init(buf, buf_size);

  // 将context挂载到open_req->data中，我们在其回调中可以用到
  open_req->data = context;

  int r = 0;
  // 首先先打开文件
  r = uv_fs_open(loop, open_req, filename, O_RDONLY, S_IRUSR, open_cb);
  if (r < 0) CHECK(r, "uv_fs_open");

  uv_run(loop, UV_RUN_DEFAULT);
}
