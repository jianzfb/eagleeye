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
class Graph;
template <class F, class ... Args>
class NodeImpl : public Node{
public:
  /**
   * @brief Construct a new Node Impl object
   * 
   * @param g 
   * @param handler 
   * @param id 
   * @param fixed 
   */
  NodeImpl(Graph* g, F* handler, int id, EagleeyeRuntime fixed=EagleeyeRuntime(EAGLEEYE_UNKNOWN_RUNTIME))
  : Node(g, id, fixed),
    m_handler(handler){
  }

  /**
   * @brief Destroy the Node Impl object
   * 
   */
  virtual ~NodeImpl(){
    delete m_handler;
  }

  /**
   * @brief run op
   * 
   * @param runtime 
   * @return float 
   */
  float fire(EagleeyeRuntime runtime=EAGLEEYE_CPU) override {
    float time = fireImpl(typename F::INS(), runtime);
    return time;
  }

  virtual void* data(int index){
    return &(this->m_output[index]);
  }

  /**
   * @brief transfer which output to device
   * 
   * @param which 
   * @param runtime 
   * @param asyn 
   */
  virtual void transfer(int which, EagleeyeRuntime runtime, bool asyn){
    this->m_output[which].transfer(runtime, asyn);
  }
  
  /**
   * @brief transfer data to device
   * 
   * @param runtime 
   */
  virtual void transfer(EagleeyeRuntime runtime, bool asyn) noexcept override{
    // current node run on 'runtime'
    for(int i = 0; i < this->data_.size(); ++i){      
      this->data_[i]->transfer(this->index_[i], runtime, asyn);
    }
  }

  /**
   * @brief get data size
   * 
   * @param index 
   * @return size_t 
   */
  size_t size(int index) noexcept override {
    return m_handler->getOutputTensor(index).blobsize();
  }

  /**
   * @brief initialize node
   * 
   * @param runtime 
   * @param data 
   */
  int init(EagleeyeRuntime runtime, std::map<std::string, std::vector<float>> data) noexcept override{
    if(!this->init_){
      // 1.step initialize hanlder
      int result = m_handler->init(data);

      // 3.step reset flag
      this->init_ = true;
      return result;
    }
  }

  bool update(typename F::Type data, int index=0){
    this->m_handler->update(data, index);
    return true;
  }

  virtual bool update(void* data, int index=0){
    // block
    this->m_handler->update(data, index);
    return true;
  }

  virtual bool fetch(void*& data, int index=0, bool block=false){
    // block / no block
    return this->m_handler->fetch(data, index, block);
  }

private:
  float fireImpl(index_sequence<>, EagleeyeRuntime runtime) {
    long start_time = EagleeyeTime::getCurrentTime();
    switch(runtime.type()){
      case EAGLEEYE_CPU:
        m_handler->runOnCpu(m_output);
        break;
      case EAGLEEYE_GPU:
        m_handler->runOnGpu(m_output);
        break;
      default:
        m_handler->runOnCpu(m_output);
        break;
    }
    long end_time = EagleeyeTime::getCurrentTime();
    EAGLEEYE_LOGD("run %s with %d us on %s ", name.c_str(), (end_time-start_time),runtime.device().c_str());
    return float(end_time-start_time);
  }

  template <std::size_t ... Is>
  float fireImpl(index_sequence<Is ...>, EagleeyeRuntime runtime) {
    std::vector<typename F::Type> input = 
          {*(typename F::Type *)data_[Is]->data(index_[Is]) ...};
    
    long start_time = EagleeyeTime::getCurrentTime();
    switch(runtime.type()){
      case EAGLEEYE_CPU:
        m_handler->runOnCpu(m_output, input);
        break;
      case EAGLEEYE_GPU:
        m_handler->runOnGpu(m_output, input);
        break;
      default:
        m_handler->runOnCpu(m_output, input);
        break;
    }
    long end_time = EagleeyeTime::getCurrentTime();
    EAGLEEYE_LOGD("run %s with %d us on %s ", name.c_str(), (end_time-start_time),runtime.device().c_str());
    return float(end_time-start_time);
  }


private:
  std::vector<typename F::Type> m_output;
  F* m_handler;
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
deduce_node_type<F, Args...> * makeNode(Graph* g, F* f, int id, EagleeyeRuntime fixed) {
  using node_type = deduce_node_type<F, Args...>;
  return new node_type(g, f, id, fixed);
}

}

}
#endif

