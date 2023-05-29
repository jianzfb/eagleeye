#ifndef _EAGLEEYE_NODE_H_
#define _EAGLEEYE_NODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include <eagleeye/engine/nano/dataflow/edge.hpp>
#include "eagleeye/common/EagleeyeRuntime.h"
#include <map>
#include <atomic>
#include <mutex>
#include <memory>
#include <utility>
#include <vector>
#include <cassert>
#include <iostream>

namespace eagleeye{
namespace dataflow {

class Node {
public:
  friend class Graph;
  friend class AsynGraph;
  std::string name;

  Node(int id, EagleeyeRuntime fixed=EagleeyeRuntime(EAGLEEYE_UNKNOWN_RUNTIME))
  :id_(id),fixed_on_runtime_(false) {
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
   * @brief run on runtime 
   */ 
  virtual int fire(EagleeyeRuntime d, void* data, int32_t& elapsed_time) noexcept = 0;

  /**
   * @brief get buffer size of node output
   */ 
  virtual size_t size(int index=0) noexcept = 0;
  
  /**
   * @brief initialize node
   */ 
  virtual int init(std::map<std::string, std::vector<float>> data) noexcept = 0;
  virtual int init(std::map<std::string, std::vector<std::vector<float>>> params) noexcept =0;
  virtual int init(std::map<std::string, std::vector<std::string>> params) noexcept = 0;

  /**
   * @brief transfer target runtime
   */ 
  virtual void transfer(int which, EagleeyeRuntime runtime, bool asyn) noexcept = 0;

  
  bool operator==(Node const & other) const noexcept {
    return this == &other;
  }

  bool operator!=(Node const & other) const noexcept {
    return !(*this == other);
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
  virtual bool update(void* data, std::vector<int64_t> shape, int index=0)=0;

  /**
   * @brief get node data
   */ 
  virtual bool fetch(void*& data, std::vector<int64_t>& shape, EagleeyeType type, int index=0, bool block=false)=0;

  /**
   * @brief Get the Dependent Num
   * 
   * @return int 
   */
  int getDependentNum(){return data_.size();}

  /**
   * @brief Get the Output object
   * 
   * @param index 
   * @param request_count 
   * @return void* 
   */
  virtual void* getOutput(int index, int request_count=0){return NULL;};

protected:
  std::atomic_uint count_;
  bool output_;

  std::vector<Edge *> next_;
  std::vector<Edge *> prev_;
  std::vector<Node *> data_;
  std::vector<int>    index_;
  std::vector<int>    order_;
  std::vector<int>    inv_order_;

  int id_;
  EagleeyeRuntime runtime_;
  bool fixed_on_runtime_;
};

}

}
#endif

