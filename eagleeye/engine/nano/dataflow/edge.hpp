#ifndef _EAGLEEYE_EDGE_H_
#define _EAGLEEYE_EDGE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include <atomic>
#include <memory>

namespace eagleeye{

namespace dataflow {
class Node;
class Edge {
public:  
  Edge (Node & prev, Node & next) noexcept
  : prev_(prev), next_(next) {
    lock_ = false;
  }

  Node & next () noexcept {
    return next_;
  }

  Node & prev () noexcept {
    return prev_;
  }

  bool lock () noexcept {
    bool f = lock_;
    lock_ = true;
    return f;
  }

  void unlock () noexcept {
    lock_ = false;
  }

  bool is_locked () const noexcept {
    return lock_;
  }
private:
  std::atomic_bool lock_;
  Node & prev_;
  Node & next_;
};

}

}
#endif

