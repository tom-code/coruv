

#include <mutex>
#include <list>
#include <functional>


template<typename T>
struct queue_t {
  std::list<T> q;
  std::mutex mutex;

  std::list<std::function<void(T)>> subscribers;

  void push(T item) {
    std::unique_lock lock(mutex);
    if (subscribers.empty()) {
      q.push_back(item);
    } else {
      auto s = subscribers.front();
      subscribers.pop_front();
      lock.unlock();
      s(item);
    }
  }

  void subscribe(std::function<void(T)> subscriber) {
    std::unique_lock lock(mutex);
    if (q.empty()) {
      subscribers.push_back(subscriber);
    } else {
      auto i = q.front();
      q.pop_front();
      lock.unlock();
      subscriber(i);
    }
  }
};

