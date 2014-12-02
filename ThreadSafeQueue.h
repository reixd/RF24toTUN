// Based on http://thisthread.blogspot.de/2011/09/threadsafe-stdqueue.html
// by egalli64@gmail.com

#include <queue>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

template <typename T>
class ThreadSafeQueue {
private:
  std::queue<T> q_;
  boost::mutex m_; // 1
  boost::condition_variable c_;

public:
  void push(const T& data) {
    boost::lock_guard<boost::mutex> l(m_); // 1
    q_.push(data);
    c_.notify_one(); // 2
  }

  T pop() {
    boost::mutex::scoped_lock l(m_); // 3
    while(q_.size() == 0) {
      c_.wait(l); // 4
    }

    T res = q_.front();
    q_.pop();
    return res; // 5
  }

  bool empty() {
    return q_.empty();
  }
  
  size_t size() {
    return q_.size();
  }
  
};
