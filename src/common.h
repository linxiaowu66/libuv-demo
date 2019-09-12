#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "uv.h"

#define CHECK(r, msg) if (r < 0) {                                                       \
  fprintf(stderr, "%s: [%s(%d): %s]\n", msg, uv_err_name((r)), (int) r, uv_strerror((r))); \
  exit(1);                                                                           \
}

typedef struct context_struct {
  uv_fs_t *open_req;
  uv_buf_t buf;
}uv_context_t;

typedef struct {
  uv_write_t req;
  uv_buf_t buf;
} write_req_t;

typedef struct {
  char string[30];
  int number;
} thread_args_t;
