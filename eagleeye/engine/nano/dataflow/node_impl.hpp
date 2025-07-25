#ifndef _EAGLEEYE_NODE_IMPL_H_
#define _EAGLEEYE_NODE_IMPL_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include <eagleeye/engine/nano/dataflow/node.hpp>
#include <eagleeye/engine/nano/dataflow/meta.hpp>
#include "eagleeye/common/EagleeyeRuntime.h"
#include "eagleeye/common/EagleeyeTime.h"
#include "eagleeye/basic/type.h"
#include <memory>
#include <tuple>
#include <vector>
#include <functional>
#include <numeric>

namespace eagleeye{
namespace dataflow{
template <class F, class ... Args>
class NodeImpl : public Node{
public:
  /**
   * @brief Construct a new Node Impl object
   * 
   * @param handler 
   * @param id 
   * @param fixed 
   */
  NodeImpl(int id, EagleeyeRuntime fixed=EagleeyeRuntime(EAGLEEYE_UNKNOWN_RUNTIME), bool is_circle=false)
  : Node(id, fixed, is_circle){}

  /**
   * @brief Destroy the Node Impl object
   * 
   */
  virtual ~NodeImpl(){
  }

  /**
   * @brief run op
   * 
   * @param runtime 
   * @return int 
   */
  int fire(EagleeyeRuntime runtime, void* data, int32_t& elapsed_time) noexcept override {
    return fireImpl(typename F::INS(), runtime, elapsed_time);
  }

  /**
   * @brief transfer which output to device
   * 
   * @param which 
   * @param runtime 
   * @param asyn 
   */
  virtual void transfer(int which, EagleeyeRuntime runtime, bool asyn) noexcept override{
    (&m_handler)->getOutput(which).transfer(runtime, asyn);
  }
  
  /**
   * @brief get data size
   * 
   * @param index 
   * @return size_t 
   */
  size_t size(int index) noexcept override {
    return (&m_handler)->getOutputSize(index);
  }

  /**
   * @brief initialize node
   * 
   * @param data 
   */
  virtual int init(std::map<std::string, std::vector<float>> data) noexcept{
      // 1.step initialize hanlder
      int result = (&m_handler)->init(data);
      return result;
  }

  virtual int init(std::map<std::string, std::vector<std::vector<float>>> data) noexcept{
    // 1.step initialize hanlder
    int result = (&m_handler)->init(data);
    return result;
  }

  virtual int init(std::map<std::string, std::vector<std::string>> data) noexcept{
    // 1.step initialize hanlder
    int result = (&m_handler)->init(data);
    return result;
  }

  virtual int init(std::map<std::string, void*> data) noexcept{
    int result = (&m_handler)->init(data);
    return result;
  }

  virtual bool update(void* data, std::vector<int64_t> shape, EagleeyeType type, int index=0){
    // block
    (&m_handler)->update(data, shape, type, index);
    return true;
  }

  virtual bool fetch(void*& data, std::vector<int64_t>& shape, EagleeyeType& type, int index=0, bool block=false){
    // block / no block
    return (&m_handler)->fetch(data, shape, type, index, block);
  }

  virtual void* getOutput(int index, int request_count=0){
    // get output
    return &((&m_handler)->getOutput(index));
  }

  virtual void clear(){
    (&m_handler)->clear();
  }

private:
  int fireImpl(index_sequence<>, EagleeyeRuntime runtime, int32_t& elapsed_time) {
    long start_time = m_time_statistics.start();
    std::vector<typename F::Type> empty;
    int rtn_code = -1;
    switch(runtime.type()){
      case EAGLEEYE_CPU:
        rtn_code = (&m_handler)->runOnCpu(empty);
        break;
      case EAGLEEYE_GPU:
        rtn_code = (&m_handler)->runOnGpu(empty);
        break;
      default:
        rtn_code = (&m_handler)->runOnCpu(empty);
        break;
    }
    long end_time = m_time_statistics.finish("Run %s with on %s.", name.c_str(), runtime.device().c_str());
    elapsed_time = (int32_t)(end_time-start_time);
    return rtn_code;
  }

  template <std::size_t ... Is>
  int fireImpl(index_sequence<Is ...>, EagleeyeRuntime runtime, int32_t& elapsed_time) {
    std::vector<typename F::Type> ordered_input;

    if(data_.size() > 0){
      std::vector<typename F::Type> unordered_input = {*((typename F::Type*)(data_[Is]->getOutput(index_[Is]))) ...};
      ordered_input = std::vector<typename F::Type>{unordered_input[inv_order_[Is]] ...};
    }

    long start_time = m_time_statistics.start();
    int rtn_code = -1;
    switch(runtime.type()){
      case EAGLEEYE_CPU:
        rtn_code = (&m_handler)->runOnCpu(ordered_input);
        break;
      case EAGLEEYE_GPU:
        rtn_code = (&m_handler)->runOnGpu(ordered_input);
        break;
      default:
        rtn_code = (&m_handler)->runOnCpu(ordered_input);
        break;
    }

    long end_time = m_time_statistics.finish("Run %s with on %s.", name.c_str(),runtime.device().c_str());
    elapsed_time = (int32_t)(end_time-start_time);
    
    return rtn_code;
  }

private:
  F m_handler;
  EagleeyeTimeStatics m_time_statistics;
};

namespace impl {
  template <class F, class ... Args>
  struct Deducenode_type {
    using type = NodeImpl<F, Args...>;
  };
};

template <class F, class ... Args>
using deduce_node_type = typename impl::Deducenode_type<F, Args...>::type;

template <class F, class ... Args>
deduce_node_type<F, Args...> * makeNode(int id, EagleeyeRuntime fixed, bool is_circle) {
  using node_type = deduce_node_type<F, Args...>;
  return new node_type(id, fixed, is_circle);
}

}
}
#endif

