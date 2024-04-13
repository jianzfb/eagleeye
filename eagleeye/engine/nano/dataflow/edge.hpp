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
  Edge(Node* prev,int prev_slot, Node* next, int next_slot) noexcept
  : prev_(prev), next_(next), prev_slot_(prev_slot), next_slot_(next_slot) {
    lock_ = false;
  }

  Node* next() noexcept {
    return next_;
  }

  Node* prev() noexcept {
    return prev_;
  }

  int pre_slot() noexcept{
    return prev_slot_;
  }
  int next_slot() noexcept{
    return next_slot_;
  }

  bool lock() noexcept {
    bool f = lock_;
    lock_ = true;
    return f;
  }

  void unlock() noexcept {
    lock_ = false;
  }

  bool is_locked() const noexcept {
    return lock_;
  }

private:
  std::atomic_bool lock_;
  Node* prev_;
  int prev_slot_;
  Node* next_;
  int next_slot_;
};
}
}
#endif

