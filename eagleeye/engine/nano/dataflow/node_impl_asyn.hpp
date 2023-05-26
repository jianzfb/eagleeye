#ifndef _EAGLEEYE_ASYN_NODE_IMPL_H_
#define _EAGLEEYE_ASYN_NODE_IMPL_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include <eagleeye/engine/nano/dataflow/node.hpp>
#include <eagleeye/engine/nano/dataflow/meta.hpp>
#include "eagleeye/engine/nano/dataflow/queue.hpp"
#include "eagleeye/common/EagleeyeRuntime.h"
#include "eagleeye/common/EagleeyeTime.h"
#include "eagleeye/basic/spinlock.hpp"
#include "eagleeye/basic/type.h"
#include <memory>
#include <tuple>
#include <vector>
#include <functional>
#include <numeric>

namespace eagleeye{
namespace dataflow{
  

template <class F, class ... Args>
class AsynNodeImpl: public Node{
public:
  /**
   * @brief Construct a new Node Impl object
   * 
   * @param handler 
   * @param id 
   * @param fixed 
   */
  AsynNodeImpl(F f, int process_num, int id, EagleeyeRuntime fixed=EagleeyeRuntime(EAGLEEYE_UNKNOWN_RUNTIME))
  : Node(id, fixed),m_thread_queues(process_num){
      this->m_robin_round     = 0;
      this->m_response_count  = 0;
      this->m_request_count   = 0;

      // 处理单元
      for(int i = 0; i < process_num; ++i){
        if(i == 0){
          this->m_processers.push_back(f);
        }
        else{
          this->m_processers.push_back(F(f));
        }
      }

      // 线程
      for(int i=0; i<process_num; ++i){
          m_run_threads.push_back(std::thread(std::bind(&AsynNodeImpl<F, Args...>::execute, this, i, fixed)));
      }
  }

  /**
   * @brief Destroy the Node Impl object
   * 
   */
  virtual ~AsynNodeImpl(){
    // 释放线程
    for(int i = 0; i < this->m_processers.size(); ++i){
      m_thread_queues[i].push(std::pair<int, void*>(-1, NULL));
    }

    for(int i = 0; i < this->m_run_threads.size(); ++i){
      if(m_run_threads[i].joinable()){
        m_run_threads[i].join();
      }
    }
  }

  /**
   * @brief run op
   * 
   * @param runtime 
   * @return float 
   */
  virtual float fire(EagleeyeRuntime runtime=EAGLEEYE_CPU, void* data=NULL) noexcept override{
    float time = fireImpl(typename F::INS(), runtime, data);
    return time;
  }

  /**
   * @brief transfer which output to device
   * 
   * @param which 
   * @param runtime 
   * @param asyn 
   */
  virtual void transfer(int which, EagleeyeRuntime runtime, bool asyn) noexcept override{
    EAGLEEYE_LOGD("Ignore.");
  }
  
  /**
   * @brief get data size
   * 
   * @param index 
   * @return size_t 
   */
  size_t size(int index) noexcept override {
    return 0;
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
    // int result = (&m_handler)->init(data);
    // return result;
    return 0;
  }

  virtual int init(std::map<std::string, std::vector<std::string>> data) noexcept{
    // 1.step initialize hanlder
    // int result = (&m_handler)->init(data);
    // return result;
    return 0;
  }
  virtual bool update(void* data, int index=0){return false;}

  /**
   * @brief dont support
   * 
   * @param data 
   * @param index 
   * @param block 
   * @return true 
   * @return false 
   */
  virtual bool fetch(void*& data, std::vector<int64_t>& shape, EagleeyeType type, int index=0, bool block=true){
    if(m_response_count <= m_request_count){
      m_lock.lock();
      typename std::map<int, std::pair<int, std::vector<typename F::Type>>>::iterator iter = m_outputs.find(m_response_count);
      while (iter == m_outputs.end()){
        m_lock.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(100));  // 0.1ms
        m_lock.lock();
        iter = m_outputs.find(m_response_count);
      }
      data = iter->second.second[index].cpu();
      m_lock.unlock();

      // 仅保留一帧缓存
      if(m_response_count >= 1){
        m_lock.lock();
        typename std::map<int, std::pair<int, std::vector<typename F::Type>>>::iterator waiting_erase_iter = m_outputs.find(m_response_count-1);
        m_outputs.erase(waiting_erase_iter);
        m_lock.unlock();
      }

      // 增加响应计数
      m_response_count += 1;
      return true;
    }

    return false;
  }

  virtual void* getOutput(int index, int request_count=0){
    // get output
    m_lock.lock();
    typename std::map<int, std::pair<int, std::vector<typename F::Type>>>::iterator iter = m_outputs.find(request_count);
    while (iter == m_outputs.end()){
      m_lock.unlock();
      std::this_thread::sleep_for(std::chrono::microseconds(100));    // 0.1ms
      m_lock.lock();
      iter = m_outputs.find(request_count);
    }
    
    typename F::Type data_on_port = iter->second.second[index];    
    iter->second.first -= 1;
    if(iter->second.first == 0){
      m_outputs.erase(iter);
    }

    m_lock.unlock();
    return &data_on_port;

  }

  virtual void clearOutput(int index, int request_count){
    // clear output
    m_lock.lock();
    typename std::map<int, std::pair<int, std::vector<typename F::Type>>>::iterator iter = m_outputs.find(request_count);
    while (iter == m_outputs.end()){
      m_lock.unlock();
      std::this_thread::sleep_for(std::chrono::microseconds(100));    // 0.1ms
      m_lock.lock();
      iter = m_outputs.find(request_count);
    }
      
    iter->second.first -= 1;
    if(iter->second.first == 0){
      m_outputs.erase(iter);
    }

    m_lock.unlock();
  }


private:
  float fireImpl(index_sequence<>, EagleeyeRuntime runtime, void* data=NULL) {
    m_thread_queues[m_robin_round].push(std::pair<int, void*>(m_request_count, data));
    m_request_count += 1;
    m_robin_round = (m_robin_round+1)%m_processers.size();
    return 0.0f;
  }

  template <std::size_t ... Is>
  float fireImpl(index_sequence<Is ...>, EagleeyeRuntime runtime, void* data=NULL) {
    m_thread_queues[m_robin_round].push(std::pair<int, void*>(m_request_count, data));
    m_request_count += 1;
    m_robin_round = (m_robin_round+1)%m_processers.size();
    return 0.0f;
  }

  void execute(int thread_id, EagleeyeRuntime runtime){
    while(true){
      std::pair<int,void*> request;
      if(!m_thread_queues[thread_id].try_pop(request)){
        // 线程无数据，进行等待
        std::this_thread::sleep_for(std::chrono::microseconds(100));  // 0.1ms
        continue;
      }

      int request_count = request.first;
      void* request_data = request.second;
      if(request_count == -1){
        // 退出
        (&m_processers[thread_id])->clear();
        break;
      }

      if(request_data != NULL){
        // 更新输入
        (&m_processers[thread_id])->update(request_data, 0);

        // 获取输出
        std::vector<typename F::Type> output;
        output.push_back((&m_processers[thread_id])->getOutput(0).clone());

        // 写入输出队列
        m_lock.lock();
        m_outputs[request_count] = std::pair<int, std::vector<typename F::Type>>(this->next_.size(), output);
        m_lock.unlock();        
        continue;
      }

      // 获取依赖数据
      std::vector<typename F::Type> ordered_input(this->index_.size());
      for(int port_i = 0; port_i < this->index_.size(); ++port_i){
        // ordered_input[order_[port_i]] = ((AsynNodeImpl<F, Args...>*)(data_[port_i]))->getOutput(this->index_[port_i], request_count);
        ordered_input[order_[port_i]] = *((typename F::Type*)(data_[port_i]->getOutput(index_[port_i], request_count)));
        data_[port_i]->clearOutput(index_[port_i], request_count);
      }      

      if(this->index_.size() == 0){
        // 无输入数据
        std::vector<typename F::Type> empty;
        switch(runtime.type()){
          case EAGLEEYE_CPU:
            (&m_processers[thread_id])->runOnCpu(empty);
            break;
          case EAGLEEYE_GPU:
            (&m_processers[thread_id])->runOnGpu(empty);
            break;
          default:
            (&m_processers[thread_id])->runOnCpu(empty);
            break;
        }
      }
      else{
        // 有输入数据
        switch(runtime.type()){
          case EAGLEEYE_CPU:
            (&m_processers[thread_id])->runOnCpu(ordered_input);
            break;
          case EAGLEEYE_GPU:
            (&m_processers[thread_id])->runOnGpu(ordered_input);
            break;
          default:
            (&m_processers[thread_id])->runOnCpu(ordered_input);
            break;
        }
      }

      // 获取输出
      std::vector<typename F::Type> output;
      for(int i=0; i<(&m_processers[thread_id])->getOutputNum(); ++i){
        output.push_back((&m_processers[thread_id])->getOutput(i).clone());
      }

      // 加锁，送入输出
      m_lock.lock();
      m_outputs[request_count] = std::pair<int, std::vector<typename F::Type>>(this->next_.size(), output);
      m_lock.unlock();
    }
  }

private:
  std::vector<F> m_processers;                                                          // 处理函数
  std::map<int, std::pair<int, std::vector<typename F::Type>>> m_outputs;               // 输出 (帧计数, <出度计数, 数据>)
  std::vector<Queue<std::pair<int, void*>>> m_thread_queues;                            // 输入
  std::vector<std::thread> m_run_threads;                                               // 线程池
  int m_robin_round;
  int m_request_count;                                                                  // 请求计数
  int m_response_count;                                                                 // 响应计数
  spinlock m_lock;
};

namespace impl {
  template <class F, class ... Args>
  struct AsynDeducenodeType {
    using type = AsynNodeImpl<F, Args...>;
  };
};

template <class F, class ... Args>
using deduce_asyn_node_type = typename impl::AsynDeducenodeType<F, Args...>::type;

template <class F, class ... Args>
deduce_asyn_node_type<F, Args...> * makeAsynNode(F f, int process_num, int id, EagleeyeRuntime fixed) {
  using asyn_node_type = deduce_asyn_node_type<F, Args...>;
  return new asyn_node_type(f, process_num, id, fixed);
}

}
}
#endif

