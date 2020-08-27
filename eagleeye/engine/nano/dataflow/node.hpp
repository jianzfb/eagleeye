#ifndef _EAGLEEYE_NODE_H_
#define _EAGLEEYE_NODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include <eagleeye/engine/nano/dataflow/edge.hpp>
#include "eagleeye/common/EagleeyeRuntime.h"
#include <atomic>
#include <mutex>
#include <memory>
#include <utility>
#include <vector>
#include <cassert>
#include <iostream>

namespace eagleeye{
namespace dataflow {

class Graph;
enum NodeType{
    DEFAULT = 0,
    ENTRY,
    EXIT
};

class Node {
public:
  friend class Graph;
  std::string label;
  std::string name;
  std::string graphviz_node_property;

  Node(Graph* g, int id, EagleeyeRuntime fixed=EagleeyeRuntime(EAGLEEYE_UNKNOWN_RUNTIME))
  :g_(g),id_(id),init_(false),fixed_on_runtime_(false) {
    if(fixed.type() != EAGLEEYE_UNKNOWN_RUNTIME){
      runtime_ = fixed;
      fixed_on_runtime_ = true;
    }
    count_ = 0;
  }
  virtual ~Node () noexcept = default;

  bool findNext (Node const* n) {
    for (Edge * e : next_) {
      assert(e);
      if (&(e->next()) == n) { return true; }
    }
    return false;
  }

  bool findPrev (Node const* n) {
    for (Edge * e : prev_) {
      assert(e);
      if (&(e->prev()) == n) { return true; }
    }
    return false;
  }

  virtual void* data(int index) = 0;
  virtual float fire(EagleeyeRuntime d=EagleeyeRuntime(EAGLEEYE_CPU)) = 0;
  virtual size_t size(int index=0) noexcept = 0;
  virtual int init(EagleeyeRuntime runtime, char* data) noexcept = 0;
  virtual void transfer(EagleeyeRuntime runtime, bool asyn) noexcept = 0;
  virtual void transfer(int which, EagleeyeRuntime runtime, bool asyn) = 0;

  int linkIndexOfNode(Node& data_node){
    for(int index=0; index<data_.size(); ++index){
      if(data_[index] == &data_node){
        return index;
      }
    }
    return -1;
  }

  bool operator== (Node const & other) const noexcept {
    return this == &other;
  }

  bool operator!= (Node const & other) const noexcept {
    return !(*this == other);
  }

  Graph* g () noexcept {
    return g_;
  }
  void setType(NodeType t){
    node_type_ = t;
  }
  NodeType getType(){
    return node_type_;
  }
  std::string getName(){
    return name;
  }
  int getId(){
    return id_;
  }
  bool isFixedOnRuntime(){
    return fixed_on_runtime_;
  }
  bool setFixedOnRuntime(EagleeyeRuntime runtime){
    runtime_ = runtime;
    fixed_on_runtime_ = true;
  }
  EagleeyeRuntime getRuntime(){
    return runtime_;
  }

protected:
  Graph*  g_;
  std::atomic_uint count_;

  std::vector<Edge *> next_;
  std::vector<Edge *> prev_;
  std::vector<Node *> data_;
  std::vector<int>    index_;

  NodeType node_type_;
  int id_;
  EagleeyeRuntime runtime_;
  bool fixed_on_runtime_;
  bool init_;
};

}

}
#endif

