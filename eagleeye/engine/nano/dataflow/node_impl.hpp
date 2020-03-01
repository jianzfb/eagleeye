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
class NodeImpl : public Node {
public:
  F handler;
  std::vector<T> value;

  NodeImpl(Graph* g, F && handler, int id, EagleeyeRuntime fixed=EagleeyeRuntime(EAGLEEYE_UNKNOWN_RUNTIME))
  : Node(g,id,fixed),
    handler(std::forward<F>(handler)){
  }

  float fire(EagleeyeRuntime runtime=EAGLEEYE_CPU) override {
    if (data_.size() != sizeof...(Args)) {
      throw "Invalid Data Node size.";
    }
    float time = fire_impl(make_index_sequence<sizeof...(Args)>(), runtime);
    return time;
  }

  void* data(EagleeyeRuntime runtime,int index) noexcept override {
    // 1.step apply transer data to device
    value[index].transfer(runtime);

    // 2.step return value
    return &value[index];
  }
  
  size_t size(int index) noexcept override {
    std::vector<int64_t> shape = handler.getOutputShape(index);
    size_t count = std::accumulate(shape.begin(), shape.end(), 1, [](int64_t a, int64_t b){return a*b;});
    return count * sizeof(typename TypeTrait<T>::Type);
  }

  void init(EagleeyeRuntime runtime, void* data) noexcept override{
    // 初始化计算节点输出数据空间
    int output_num = handler.getOutputNum();
    for(int i=0; i<output_num; ++i){
      value[i] = T(handler.getOutputShape(i), runtime, data, false);
    }
    // 设置初始化标记
    this->init_ = true;
  }

private:
  using arg_tuple = std::tuple<Args ...>;
  float fire_impl(index_sequence<>, EagleeyeRuntime runtime) {
    if(!this->init_){
      // 生成随机数据
      int output_num = handler.getOutputNum();
      value.reserve(output_num);
      for(int index=0; index<output_num; ++index){
        value[index] = T(handler.getOutputShape(index), runtime);
      }

      EAGLEEYE_LOGD("generate random data for %s node on %s", name.c_str(), runtime.device().c_str());
    }

    long start_time = EagleeyeTime::getCurrentTime();
    switch(runtime.type()){
      case EAGLEEYE_CPU:
        handler.run_on_cpu(value);
        break;
      case EAGLEEYE_GPU:
        handler.run_on_gpu(value);
        break;
      default:
        handler.run_on_cpu(value);
        break;
    }
    long end_time = EagleeyeTime::getCurrentTime();
    EAGLEEYE_LOGD("run %s with %d us on %s ", name.c_str(), (end_time-start_time),runtime.device().c_str());
    return float(end_time-start_time);
  }

  template <std::size_t ... Is>
  float fire_impl(index_sequence<Is ...>, EagleeyeRuntime runtime) {
    std::vector<typename std::tuple_element<0, arg_tuple>::type> input = {*reinterpret_cast<typename std::tuple_element<Is, arg_tuple>::type *>(data_[Is]->data(runtime, index_[Is])) ...};
    if(!this->init_){
      // 生成随机数据
      int output_num = handler.getOutputNum();
      value.reserve(output_num);
      for(int index=0; index<output_num; ++index){
        value[index] = T(handler.getOutputShape(index), runtime);
      }

      EAGLEEYE_LOGD("generate random data for %s node on %s", name.c_str(), runtime.device().c_str());
    }

    long start_time = EagleeyeTime::getCurrentTime();
    switch(runtime.type()){
      case EAGLEEYE_CPU:
        handler.run_on_cpu(value, input);
        break;
      case EAGLEEYE_GPU:
        handler.run_on_gpu(value, input);
        break;
      default:
        handler.run_on_cpu(value, input);
        break;
    }
    long end_time = EagleeyeTime::getCurrentTime();
    EAGLEEYE_LOGD("run %s with %d us on %s ", name.c_str(), (end_time-start_time),runtime.device().c_str());
    return float(end_time-start_time);
  }
};

template <class F, class ... Args>
class NodeImpl<F, void, Args ...> : public Node {
public:
  F handler;
  NodeImpl(Graph* g, F && handler, int id, EagleeyeRuntime fixed=EagleeyeRuntime(EAGLEEYE_UNKNOWN_RUNTIME))
  : Node(g,id, fixed),
    handler(std::forward<F>(handler)){
  }  

  float fire(EagleeyeRuntime runtime=EAGLEEYE_CPU) override {
    if (data_.size() != sizeof...(Args)) {
      throw "Invalid Data Node size.";
    }
    float time = fire_impl(make_index_sequence<sizeof...(Args)>(), runtime);
    return time;
  }

  void* data(EagleeyeRuntime d=EAGLEEYE_CPU, int index=0) noexcept override {
    return nullptr;
  }
  size_t size(int index=0) noexcept override {
    return 0;
  }
  void init(EagleeyeRuntime runtime, void* data) noexcept override{
    // do nothing
    init_ = true;
  }

private:
  using arg_tuple = std::tuple<Args ...>;

  float fire_impl (index_sequence<>, EagleeyeRuntime runtime) {
    long start_time = EagleeyeTime::getCurrentTime();
    switch(runtime.type()){
      case EAGLEEYE_CPU:
        handler.run_on_cpu();
        break;
      case EAGLEEYE_GPU:
        handler.run_on_gpu();
        break;
      default:
        handler.run_on_cpu();
        break;
    }
    long end_time = EagleeyeTime::getCurrentTime();
    EAGLEEYE_LOGD("run %s with %d us on %s ", name.c_str(), (end_time-start_time),runtime.device().c_str());
    return float(end_time-start_time);
  }

  template <std::size_t ... Is>
  float fire_impl (index_sequence<Is ...>, EagleeyeRuntime runtime) {
    std::vector<typename std::tuple_element<0, arg_tuple>::type> input = {*reinterpret_cast<typename std::tuple_element<Is, arg_tuple>::type *>(data_[Is]->data(runtime, index_[Is])) ...};
    long start_time = EagleeyeTime::getCurrentTime();
    switch(runtime.type()){
      case EAGLEEYE_CPU:
        handler.run_on_cpu(input);
        break;
      case EAGLEEYE_GPU:
        handler.run_on_gpu(input);
        break;
      default:
        handler.run_on_cpu(input);
        break;
    }
    long end_time = EagleeyeTime::getCurrentTime();
    EAGLEEYE_LOGD("run %s with %d us on %s ", name.c_str(), (end_time-start_time),runtime.device().c_str());
    return float(end_time-start_time);
  }
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
deduce_node_type<F, T, Args...> * make_node(Graph* g, F && f, int id, EagleeyeRuntime fixed) {
  using node_type = deduce_node_type<F, T, Args...>;
  return new node_type(g, std::forward<F>(f), id, fixed);
}

}

}
#endif

