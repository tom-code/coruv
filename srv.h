#include <functional>
#include <string>

class connection_t {

public:
  virtual void set_read_cb(std::function<void(const unsigned char *data, int size)> cb) = 0;
  virtual void set_release_cb(std::function<void()> cb) = 0;
  virtual void write(const unsigned char *data, int size) = 0;
  virtual void close() = 0;
  virtual ~connection_t(){};
};

void server(std::string bind_ip, int port, std::function<void(connection_t*)> c);
