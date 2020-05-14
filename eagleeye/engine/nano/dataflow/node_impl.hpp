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
template <class F, class T, class ... Args>
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
  NodeImpl(Graph* g, 
            F && handler, 
            int id, 
            EagleeyeRuntime fixed=EagleeyeRuntime(EAGLEEYE_UNKNOWN_RUNTIME))
  : Node(g,id,fixed),
    handler(std::forward<F>(handler)){
  }

  /**
   * @brief Destroy the Node Impl object
   * 
   */
  virtual ~NodeImpl(){}

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

  /**
   * @brief get data
   * 
   * @param runtime 
   * @param index 
   * @return void* 
   */
  virtual void* data(int index) noexcept override{
    return &(value[index]);
  }
  
  /**
   * @brief transfer data to device
   * 
   * @param runtime 
   */
  virtual void transfer(EagleeyeRuntime runtime, bool asyn) noexcept override{
    for(int i=0; i<data_.size(); ++i){      
      (*(T*)(data_[i]->data(index_[i]))).transfer(runtime, asyn);
    }
  }

  /**
   * @brief get data size
   * 
   * @param index 
   * @return size_t 
   */
  size_t size(int index) noexcept override {
    std::vector<int64_t> shape = handler.getOutputShape(index);
    size_t count = std::accumulate(shape.begin(), shape.end(), 1, [](int64_t a, int64_t b){return a*b;});
    return count * sizeof(typename TypeTrait<T>::Type);
  }

  /**
   * @brief initialize node
   * 
   * @param runtime 
   * @param data 
   */
  void init(EagleeyeRuntime runtime, void* data) noexcept override{
    if(!this->init_){
      // 1.step initialize hanlder
      handler.init(data);

      // 2.step allocate space
      int output_num = handler.getOutputNum();
      value.resize(output_num);
      for(int i=0; i<output_num; ++i){
        value[i] = T(handler.getOutputShape(i), runtime);
      }

      // 3.step reset flag
      this->init_ = true;
    }
  }

private:
  float fireImpl(index_sequence<>, EagleeyeRuntime runtime) {
    long start_time = EagleeyeTime::getCurrentTime();
    switch(runtime.type()){
      case EAGLEEYE_CPU:
        handler.runOnCpu(value);
        break;
      case EAGLEEYE_GPU:
        handler.runOnGpu(value);
        break;
      default:
        handler.runOnCpu(value);
        break;
    }
    long end_time = EagleeyeTime::getCurrentTime();
    EAGLEEYE_LOGD("run %s with %d us on %s ", name.c_str(), (end_time-start_time),runtime.device().c_str());
    return float(end_time-start_time);
  }

  template <std::size_t ... Is>
  float fireImpl(index_sequence<Is ...>, EagleeyeRuntime runtime) {
    std::vector<typename F::Type> input = 
          {*(T*)data_[Is]->data(index_[Is]) ...};

    long start_time = EagleeyeTime::getCurrentTime();
    switch(runtime.type()){
      case EAGLEEYE_CPU:
        handler.runOnCpu(value, input);
        break;
      case EAGLEEYE_GPU:
        handler.runOnGpu(value, input);
        break;
      default:
        handler.runOnCpu(value, input);
        break;
    }
    long end_time = EagleeyeTime::getCurrentTime();
    EAGLEEYE_LOGD("run %s with %d us on %s ", name.c_str(), (end_time-start_time),runtime.device().c_str());
    return float(end_time-start_time);
  }

private:
  F handler;
  std::vector<T> value;
};

namespace impl {
  template <class F, class T, class ... Args>
  struct Deducenode_type {
    using type = NodeImpl<F, T, Args...>;
  };
};

template <class F, class T, class ... Args>
using deduce_node_type = typename impl::Deducenode_type<F, T, Args...>::type;

template <class T, class F, class ... Args>
deduce_node_type<F, T, Args...> * makeNode(Graph* g, F && f, int id, EagleeyeRuntime fixed) {
  using node_type = deduce_node_type<F, T, Args...>;
  return new node_type(g, std::forward<F>(f), id, fixed);
}

}

}
#endif

