#ifndef _EAGLEEYE_GRAPH_H_
#define _EAGLEEYE_GRAPH_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/engine/nano/dataflow/edge.hpp"
#include "eagleeye/engine/nano/dataflow/node.hpp"
#include "eagleeye/engine/nano/dataflow/node_impl.hpp"
#include "eagleeye/engine/nano/dataflow/queue.hpp"
#include "eagleeye/engine/nano/dataflow/worker.hpp"
#include "eagleeye/engine/nano/dataflow/schedule.h"
#include "eagleeye/engine/nano/dataflow/heft.h"
#include "eagleeye/engine/nano/dataflow/noschedule.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/basic/spinlock.hpp"
#include "eagleeye/basic/Tensor.h"
#include <atomic>
#include <iostream>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <vector>

namespace eagleeye{
namespace dataflow{
enum ScheduleType{
  NO_SCHEDULE       = 0,
  HEFT_SCHEDULE     = 1,
  DYNAMIC_SCHEDULE  = 2
};

class Graph {
public:  
  /**
   * @brief Construct a new Graph object
   * 
   * @param runtimes 
   * @param num_worker 
   * @param schedule_type 
   */
  Graph(std::vector<EagleeyeRuntime> runtimes, 
        std::size_t num_worker = 2,
        ScheduleType schedule_type=NO_SCHEDULE)
    :m_queue(num_worker) {
    // 1.step build worker
    for (std::size_t i = 0; i < num_worker; ++i) {
      m_worker.emplace_back(i);
    }

    // 2.step build resource schedule 
    switch (schedule_type){
      case NO_SCHEDULE:
        m_schedule = new NoSchedule(this,runtimes);
        break;
      case HEFT_SCHEDULE:
        m_schedule = new HEFT(this, runtimes);
        break;
      default:
        m_schedule = new NoSchedule(this,runtimes);
        break;
    }  

    // 3.step others
    m_waiting_stop_count = 0;
  }

  /**
   * @brief Destroy the Graph object
   * 
   */
  virtual ~Graph () {
    for (Node * n : m_nodes) {
      delete n;
    }
    for (Edge * e : m_edges) {
      delete e;
    }

    delete m_schedule;
  }

  /**
   * @brief analyze DAG
   * 
   */
  void analyze(){
    // // 1.step analyze compute resource
    // EAGLEEYE_LOGD("Analyze DAG Structure and Resource");
    // this->m_schedule->collectStatistic();
    // this->m_schedule->analyze();

    // // 2.step print log
    // EAGLEEYE_LOGD("DAG Task Execute Schedule");
    // for(Node* n: m_nodes){
    //   EagleeyeRuntime d = this->m_schedule->getRuntime(n);
    //   if(d.type() == EAGLEEYE_CPU){
    //     EAGLEEYE_LOGD("node %s (%d) on CPU", n->getName().c_str(), n->getId());
    //   }
    //   else if(d.type() == EAGLEEYE_GPU){
    //     EAGLEEYE_LOGD("node %s (%d) on GPU", n->getName().c_str(), n->getId());
    //   }
    //   else{
    //     EAGLEEYE_LOGE("node %s (%d) on UNKNOWN device", n->getName().c_str(), n->getId());
    //   }
    // }
  }
  
  /**
   * @brief init DAG 
   * 
   * @param model_path 
   */
  void init(const char* model_path){
    // 1.step load model path
    if(model_path != NULL){
      // 从模型文件加载模型结构
      // 加载模型DAG，和参数
    }

    // 2.step init node
    for(Node* n: m_nodes){
      EagleeyeRuntime runtime = this->m_schedule->getRuntime(n);
      std::map<std::string, std::vector<float>> data;
      n->init(runtime, data);
    }
  }

  /**
   * @brief get worker thread number
   * 
   * @return std::size_t 
   */
  std::size_t numWorker () const noexcept {
    return m_worker.size();
  }

  /**
   * @brief start worker thread
   * 
   */
  void run(std::map<std::string, void*> inputs, std::map<std::string, void*>& outputs){
    // 1.step check
    if(outputs.size() == 0){
      EAGLEEYE_LOGD("output node empty, skip run");
      return;
    }
    
    // 2.step update input data
    std::map<std::string, void*>::iterator in_iter,in_iend(inputs.end());
    for(in_iter = inputs.begin(); in_iter != in_iend; ++in_iter){
      if(m_nodes_map.find(in_iter->first) != m_nodes_map.end()){
        m_nodes_map[in_iter->first]->update(in_iter->second, 0);
      }
      else{
        EAGLEEYE_LOGD("ignore %s node", in_iter->first.c_str());
      }
    }

    // 3.step reset output flag
    std::map<std::string, bool> output_map;
    std::map<std::string, void*>::iterator out_iter,out_iend(outputs.end());
    for(out_iter = outputs.begin(); out_iter != out_iend; ++out_iter){
      m_nodes_map[out_iter->first]->output_ = true;
    }
    m_exit_node_num = outputs.size();

    // 4.step push to work queue    
    m_stop = false;
    m_waiting_stop_count = 0;
    m_queue = std::vector<Queue<Node*>>(this->numWorker());
    int queue_index = 0;
    for(auto & n : m_entry_nodes){
        m_queue[queue_index%this->numWorker()].push(n);
        queue_index += 1;
    }

    // 5.step launch thread
    // TODO: using thread pool
    for (auto & worker : m_worker) {
      worker.thread = std::thread([&](){work(worker.id);});
    }

    // 6.step waiting finish
    waitingUntilFinish(outputs);
  }

  template <class F, class ... Args>
  deduce_node_type<F, Args...>* add(std::string name, 
                                    F* f, 
                                    EagleeyeRuntime fixed=EagleeyeRuntime(EAGLEEYE_UNKNOWN_RUNTIME)) {
    // 1.step check duplication of name
    if(name == ""){
      name = "node";
    }
    if(m_nodes_map.find(name) != m_nodes_map.end()){
      std::unordered_map<std::string, Node*>::iterator iter, iend(m_nodes_map.end());

      int duplicate_count = 0;
      for(iter = m_nodes_map.begin(); iter!=iend; ++iter){
        if(startswith(iter->first, name)){
          duplicate_count += 1;
        }
      }

      name = name + "/" + tos(duplicate_count);
    }

    // 2.step build node
    auto * n = makeNode<F, Args...>(this, f, m_nodes.size(), fixed);
    n->name = name;
    m_nodes.push_back(n);
    m_nodes_map[name] = n;

    if(f->getInputNum() == 0){
      m_entry_nodes.push_back(n);
    }
    return n;
  }

  void bind(Node* from, int index, Node* to) {
    assert(!from->findNext(to));  
    assert(!to->findPrev(from));
    Edge * e = new Edge(*from, *to);

    from->next_.push_back(e);
    to->prev_.push_back(e);
    m_edges.push_back(e);

    to->data_.push_back(from);
    to->index_.push_back(index);
  }

  Node* findNode(std::string const & name) {
    auto it = m_nodes_map.find(name);
    if (it == m_nodes_map.end()) { return NULL; }
    return it->second;
  }

  std::vector<Node*> getNodes(){
    return this->m_nodes;
  }

  std::vector<Edge*> getEdges(){
    return this->m_edges;
  }

  int size(){
    return this->m_nodes.size();
  }

private:
  std::vector<Node*> m_nodes;
  std::vector<Edge*> m_edges;
  std::unordered_map<std::string, Node*> m_nodes_map;
  std::vector<Node*> m_entry_nodes;
  int m_exit_node_num;

  std::vector<Queue<Node*>> m_queue;
  std::vector<Worker> m_worker;
  std::atomic_bool m_stop;
  std::atomic_int m_waiting_stop_count;  
  Schedule* m_schedule;

  void work(std::size_t id) {
    while (!m_stop) {
      Node * n = nullptr;
      bool flag = false;
      for (std::size_t i = 0; i < numWorker(); ++i) {
        std::size_t index = (i + id) % numWorker();
        
        if (m_queue[index].try_pop(n)) {
          flag = true;
          this->__fire(id, n);
          break;
        }
      }

      if (!flag) {
        std::this_thread::sleep_for(std::chrono::milliseconds(128));
      }
    }
  }

  void __fire(std::size_t id, Node* node) {
    // 1.step reset
    node->count_ = 0;
    
    // 2.step schedule 
    EagleeyeRuntime runtime = this->m_schedule->getRuntime(node);

    // 3.step execute node
    node->fire(runtime);

    // 4.step check whether stop
    if(this->finish(node)){
      return;
    }

    // 5.step active succeed nodes
    int i = id;
    for (Edge* next: node->next_) {
      // 5.1.step increment 1, for succeed node
      unsigned n = next->next().count_.fetch_add(1);

      // 5.2.step check, whether all dependents have been finish
      if (next->next().prev_.size() == (n + 1)){
        // 5.2.1.step get node target runtime, and launch transfer asyn
        Node* next_node = &next->next();
        EagleeyeRuntime target_runtime = this->m_schedule->getRuntime(next_node);
        next_node->transfer(target_runtime, true);

        // 5.2.2.step push to queue, prepare to execute
        m_queue[i % numWorker()].push(next_node);
        ++i;
      }
    }
  }

  /**
   * @brief finish node 
   * 
   * @param node
   */
  bool finish(Node* node){
    if(node->output_){
      m_waiting_stop_count += 1;
      node->output_ = false;
    }

    if(m_waiting_stop_count == m_exit_node_num){
      m_stop = true;
      return true;
    }

    return false;
  }

  /**
   * @brief wating untile finish
   */
  void waitingUntilFinish(std::map<std::string, void*>& outputs) {
    // 1.step waiting thread stop
    for (auto & worker : m_worker) {
      if (worker.thread.joinable()) {
        worker.thread.join();
      }
    }

    // 2.step output
    std::map<std::string, void*>::iterator out_iter,out_iend(outputs.end());
    std::map<std::string, void*> result;
    for(out_iter = outputs.begin(); out_iter != out_iend; ++out_iter){
      void* data = NULL;
      m_nodes_map[out_iter->first]->fetch(data, 0, true);
      result[out_iter->first] = data;
    }

    outputs = result;
  }
};
}
}
#endif
