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
class Node {
public:
  friend class Graph;
  std::string label;
  std::string name;

  Node(Graph* g, int id, EagleeyeRuntime fixed=EagleeyeRuntime(EAGLEEYE_UNKNOWN_RUNTIME))
  :g_(g),id_(id),init_(false),fixed_on_runtime_(false) {
    if(fixed.type() != EAGLEEYE_UNKNOWN_RUNTIME){
      runtime_ = fixed;
      fixed_on_runtime_ = true;
    }
    count_ = 0;
    output_ = false;
  }
  virtual ~Node () noexcept = default;

  /**
   * @brief get succeed nodes
   */ 
  bool findNext (Node const* n) {
    for (Edge * e : next_) {
      assert(e);
      if (&(e->next()) == n) { return true; }
    }
    return false;
  }

  /**
   * @brief get pre nodes
   */ 
  bool findPrev (Node const* n) {
    for (Edge * e : prev_) {
      assert(e);
      if (&(e->prev()) == n) { return true; }
    }
    return false;
  }

  /**
   * @brief get data ptr
   */ 
  virtual void* data(int index) = 0;

  /**
   * @brief run on runtime 
   */ 
  virtual float fire(EagleeyeRuntime d=EagleeyeRuntime(EAGLEEYE_CPU)) = 0;

  /**
   * @brief get buffer size of node output
   */ 
  virtual size_t size(int index=0) noexcept = 0;
  
  /**
   * @brief initialize node
   */ 
  virtual int init(EagleeyeRuntime runtime, std::map<std::string, std::vector<float>> data) noexcept = 0;

  /**
   * @brief transfer target runtime
   */ 
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

  int getId(){
    return id_;
  }

  /**
   * @brief check is fixed runtime
   */ 
  bool isFixedOnRuntime(){
    return fixed_on_runtime_;
  }

  /**
   * @brief set fixed runtime
   */ 
  bool setFixedOnRuntime(EagleeyeRuntime runtime){
    runtime_ = runtime;
    fixed_on_runtime_ = true;
  }

  /**
   * @brief get node scheduled runtime
   */ 
  EagleeyeRuntime getRuntime(){
    return runtime_;
  }

  /*
   * @brief update node data
   */
  virtual bool update(void* data, int index=0)=0;

  /**
   * @brief get node data
   */ 
  virtual bool fetch(void*& data, int index=0, bool block=false)=0;

protected:
  Graph*  g_;
  std::atomic_uint count_;
  bool output_;

  std::vector<Edge *> next_;
  std::vector<Edge *> prev_;
  std::vector<Node *> data_;
  std::vector<int>    index_;

  int id_;
  EagleeyeRuntime runtime_;
  bool fixed_on_runtime_;
  bool init_;
};

}

}
#endif

