#include <concepts>
#include <experimental/coroutine>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <vector>
#include "srv.h"
#include "queue.h"


struct task {
  struct promise_type {
    task get_return_object() { return {}; }
    std::experimental::suspend_never initial_suspend() { printf("initial\n"); return {}; }
    std::experimental::suspend_never final_suspend() noexcept { printf("final\n"); return {}; }
    void return_void() { printf("return\n");}
    void unhandled_exception() {}
  };
};


struct xconnection {
  struct awaitable {
    std::experimental::coroutine_handle<> _h;
    bool await_ready() { return false; }
    void await_suspend(std::experimental::coroutine_handle<> h) {
      _h = h;
      xc.q.subscribe([this](std::string d) {
      new_data = d;
        _h.resume();
      });
    }
    std::string new_data;
    std::string await_resume() { printf("call resume\n"); return new_data;}

    xconnection &xc;
    awaitable(xconnection &xci) : xc(xci) {};
    ~awaitable() {
      printf("~xconnection.awaitable\n");
    }
  };
  bool started = false;
  queue_t<std::string> q;

  awaitable wait_for_data() {
    return awaitable(*this);
  }

  connection_t *c;
  xconnection(connection_t *ci) : c(ci) {
    c->set_read_cb([this](const unsigned char *data, int size) {
      std::string dat((char*)data, size);
      q.push(dat);
    });
  }

  void send(const std::string &data) {
    c->write((unsigned char*)data.data(), data.size());
  }
  void close() {
    c->close();
  }
  ~xconnection() {
    printf("~xconnection\n");
  }
};

struct xserver {
  struct awaitable {
    std::experimental::coroutine_handle<> _h;
    bool await_ready() { return false; }
    void await_suspend(std::experimental::coroutine_handle<> h) {
      _h = h;

      s.q.subscribe([this](connection_t *c) {
        new_connection = c;
        _h.resume();
      });
    }
    connection_t *new_connection = nullptr;
    connection_t *await_resume() { return new_connection;}
    xserver &s;
    awaitable (xserver &si):  s(si) {};
  };

  bool started = false;
  queue_t<connection_t*> q;
  awaitable wait_for_connection() {
    return awaitable(*this);
  }

  void start() {
    std::thread t([this] {
      server("0.0.0.0", 3333, [this](connection_t *c) {
        q.push(c);
      });
    });
    t.detach();
  }
};

task run_connection(connection_t *c) {
  xconnection con(c);
  while (true) {
    auto p = co_await con.wait_for_data();
    printf("received data: [%s]\n", p.c_str());
    con.send(p);
    if (p.find("quit") != std::string::npos) {
      con.close();
      break;
    }
  }
}

task run() {
  xserver srv;
  srv.start();
  while (true) {
    auto connection = co_await srv.wait_for_connection();
    run_connection(connection);
  }
};

int main() {
  run();
//  while (true) {
    sleep(10);
//  }

}

