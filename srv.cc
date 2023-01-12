
#include <stdio.h>
#include <stdlib.h>
#include <uv.h>
#include <functional>

#include "srv.h"

//wrapper for tcp server based on libuv - see net.h for external interface

static void close_callback(uv_handle_t *handle);

static void write_complete(uv_write_t *req, int status) {
  free(req);
}

class connection_uv_t : public connection_t {
  uv_tcp_t *c = nullptr;
  std::function<void(const unsigned char *data, int size)> read_cb;
  std::function<void()> release_cb;
public:
  connection_uv_t(uv_tcp_t *_c) {
    c = _c;
  }

  virtual void set_read_cb(std::function<void(const unsigned char *data, int size)> cb) override {
    read_cb = cb;
  }
  virtual void set_release_cb(std::function<void()> cb) override {
    release_cb = cb;
  }

  virtual void write(const unsigned char *data, int size) override {
    uv_buf_t buf;
    buf.len = size;
    buf.base = (char*)data;

    //some examples suggests req can be on stack - but this is not true
    uv_write_t *req = (uv_write_t*)malloc(sizeof(uv_write_t));
    uv_write(req, (uv_stream_t*)c, &buf, 1, write_complete);
  }

  virtual void close() override {
    uv_close((uv_handle_t*)c, close_callback);
  }

  void data_in(unsigned char *data, int len) {
    if (read_cb) read_cb(data, len);
  }
  void release() {
    if (release_cb) release_cb();
  }

  virtual ~connection_uv_t() {
    printf("destruct\n");
  };

};

static void close_callback(uv_handle_t *handle) {
  connection_uv_t *c = (connection_uv_t*)handle->data;
  c->release(); //user release callbacks
  delete c;
  free(handle);
}

static void alloc_callback(uv_handle_t *h, size_t size, uv_buf_t *buf) {
  buf->base = (char*)malloc(size);
  buf->len = size;
}

static void receive_callback(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
  if (nread < 0) {
    uv_close((uv_handle_t*)stream, close_callback);
    if (buf) free(buf->base);
    return;
  }

  connection_uv_t *c = (connection_uv_t*)stream->data;
  c->data_in((unsigned char*)buf->base, nread);
  free(buf->base);
}


static uv_loop_t *loop;

struct server_ctx_t {
  std::function<void(connection_t*)> c;
};
static void concb(uv_stream_t *server, int status) {
  printf("got con\n");
  uv_tcp_t *c = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
  uv_tcp_init(loop, c);
  uv_accept(server, (uv_stream_t*)c);

  connection_t *con = new connection_uv_t(c);
  c->data = con;

  uv_read_start((uv_stream_t*)c, alloc_callback, receive_callback);

  server_ctx_t *cx = (server_ctx_t*)server->data;
  cx->c(con);
}



void server(std::string bind_ip, int port, std::function<void(connection_t*)> c) {
  uv_tcp_t server;
  loop = uv_default_loop();

  struct sockaddr_in addr;
  uv_ip4_addr(bind_ip.c_str(), port, &addr);

  uv_tcp_init(loop, &server);
  uv_tcp_bind(&server, (sockaddr*)&addr, 0);
  server_ctx_t *ctx = new server_ctx_t(); /////////!!!!!!!!!!!!!!!!!! fix memory leak
  ctx->c = c;
  server.data = (void*)ctx;

  uv_listen((uv_stream_t*)&server, 10, concb);

  uv_run(loop, UV_RUN_DEFAULT);  
}


