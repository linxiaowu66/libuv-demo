/*
 * 读写文件的基本步骤(使用异步形式)：
 * 1、首先初始化open请求：uv_fs_t => malloc分配或者直接定义
 * 2、使用open请求调用uv_fs_open打开文件，并传入回调
 * 3、回调中再重复上述动作继续分别调用uv_fs_read、uv_fs_close、uv_fs_write等方法
 * 4、所有操作结束之后，记得释放所有的文件请求：uv_fs_req_cleanup，并释放分配过的所有内存：free
 */
#include <stdio.h>
#include "uv.h"
#include "common.h"

static const char *filename = "/Users/linxiaowu/Github/libuv-demo/src/test.txt";

void read_cb(uv_fs_t *read_req) {
  int r = 0;
  CHECK(read_req->result, "read_cb");

   uv_context_t *context = read_req->data;

  fprintf(stderr, "[%d] => %s", uv_os_getpid(), context->buf.base);

  // 关闭文件
  uv_fs_t close_req;
  r = uv_fs_close(uv_default_loop(), &close_req, context->open_req->result, NULL);

  CHECK(r, "uv_fs_close");

  // 操作完成，记得释放所有用到的内存
  uv_fs_req_cleanup(&close_req);
  uv_fs_req_cleanup(context->open_req);
  uv_fs_req_cleanup(read_req);

  printf("[%d] => close all requests and handles successfully!\n", uv_os_getpid());

  free(context->buf.base);
  free(context);
}

void open_cb(uv_fs_t* open_req) {
  CHECK(open_req->result, "open_cb");

  fprintf(stderr, "[%d] => opened %s, result: %zd\n", uv_os_getpid(), open_req->path, open_req->result);

  uv_fs_t *read_req = malloc(sizeof(uv_fs_t));

  read_req->data = open_req->data;
  uv_context_t *context = open_req->data;

  int r = 0;
  /* 读取文件
   * 这里有个很神奇的地方：offset这个参数决定了我调用系统层的哪个函数，当我传了-1的时候，通过查看代码发现
   * 有两种组合，即下面的代码:
   * if (req->nbufs == 1)
   *    result = read(req->file, req->bufs[0].base, req->bufs[0].len);
   *  else
   *    result = readv(req->file, (struct iovec*) req->bufs, req->nbufs);
   * 然后nbufs = 1的时候，调用read函数没问题，但是如果调用下面的readv，那么就会报错：EINVAL(-22): invalid argument
   * 代码调试跟踪并阅读linux关于这个函数的说明并不能找到问题的原因
   */
  r = uv_fs_read(uv_default_loop(), read_req, open_req->result, &context->buf, context->buf.len, 0, read_cb);
  CHECK(r, "open_cb");
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
  CHECK(r, "uv_fs_open");

  uv_run(loop, UV_RUN_DEFAULT);
}
