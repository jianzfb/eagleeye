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
#include "eagleeye/basic/spinlock.hpp"
#include "eagleeye/basic/TensorX.h"
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
        std::size_t num_worker = 4,
        ScheduleType schedule_type=NO_SCHEDULE)
    :queue_(num_worker) {
    // 1.step build worker
    for (std::size_t i = 0; i < num_worker; ++i) {
      worker_.emplace_back(i);
    }

    // 2.step build resource schedule 
    switch (schedule_type){
      case NO_SCHEDULE:
        schedule_ = new NoSchedule(this,runtimes);
        break;
      case HEFT_SCHEDULE:
        schedule_ = new HEFT(this, runtimes);
        break;
      default:
        schedule_ = new NoSchedule(this,runtimes);
        break;
    }  

    // 3.step others
    exit_node_num_ = 0;
    waiting_stop_count_ = 0;
  }

  /**
   * @brief Destroy the Graph object
   * 
   */
  virtual ~Graph () {
    for (Node * n : nodes_) {
      delete n;
    }
    for (Edge * e : edges_) {
      delete e;
    }

    delete schedule_;
  }

  /**
   * @brief analyze DAG
   * 
   */
  void analyze(){
    // 1.step analyze compute resource
    EAGLEEYE_LOGD("Analyze DAG Structure and Resource");
    this->schedule_->collectStatistic();
    this->schedule_->analyze();

    // 2.step print log
    EAGLEEYE_LOGD("DAG Task Execute Schedule");
    for(Node* n: nodes_){
      EagleeyeRuntime d = this->schedule_->getRuntime(n);
      if(d.type() == EAGLEEYE_CPU){
        EAGLEEYE_LOGD("node %s (%d) on CPU", n->getName().c_str(), n->getId());
      }
      else if(d.type() == EAGLEEYE_GPU){
        EAGLEEYE_LOGD("node %s (%d) on GPU", n->getName().c_str(), n->getId());
      }
      else{
        EAGLEEYE_LOGE("node %s (%d) on UNKNOWN device", n->getName().c_str(), n->getId());
      }
    }
  }
  
  /**
   * @brief init DAG 
   * 
   * @param model_path 
   */
  void init(const char* model_path, char* data=NULL){
    EAGLEEYE_LOGD("init DAG with %s", model_path);
    // 1.step load model path
    char* model_data = data;
    if(model_path != NULL){
      // 从模型文件加载
    }

    // 2.step init node
    for(Node* n: nodes_){
      EagleeyeRuntime d = this->schedule_->getRuntime(n);
      int offset = n->init(d, model_data);
      model_data += offset;
    }
  }

  /**
   * @brief get worker thread number
   * 
   * @return std::size_t 
   */
  std::size_t numWorker () const noexcept {
    return worker_.size();
  }

  /**
   * @brief start worker thread
   * 
   */
  void run(std::map<std::string, void*> inputs, std::map<std::string, void*> outputs){
    // 1.step check
    exit_node_num_ = outputs.size();
    if(exit_node_num_ == 0){
      EAGLEEYE_LOGD("dont set end node");
      return;
    }
    
    // 2.step update input and set output flag
    std::map<std::string, void*>::iterator in_iter,in_iend(inputs.end());
    for(in_iter = inputs.begin(); in_iter != in_iend; ++in_iter){
      if(nodes_map_.find(in_iter->first) != nodes_map_.end()){
        nodes_map_[in_iter->first]->update(in_iter->second, 0);
      }
    }
    std::map<std::string, void*>::iterator out_iter,out_iend(outputs.end());
    for(out_iter = outputs.begin(); out_iter != out_iend; ++out_iter){
      nodes_map_[out_iter->first]->output_ = true;
    }

    // 3.step reset running flag
    stop_ = false;
    waiting_stop_count_ = 0;
    for(auto & n: nodes_){
      n->count_ = 0;
    }

    // 4.step push to work queue    
    queue_ = std::vector<Queue<Node*>>(this->numWorker());
    int queue_index = 0;
    for(auto & n : entry_nodes_){
        queue_[queue_index%this->numWorker()].push(n);
        queue_index += 1;
    }

    // 5.step start thread
    // TODO: using thread pool
    for (auto & worker : worker_) {
      worker.thread = std::thread([&](){work(worker.id);});
    }

    // 6.step waiting finish
    waitingUntilFinish();

    // 7.step assign output
    for(out_iter = outputs.begin(); out_iter != out_iend; ++out_iter){
      void* data = NULL;
      nodes_map_[out_iter->first]->fetch(data,0,true);
      out_iter->second = data;
    }
  }

  template <class F, class ... Args>
  deduce_node_type<F, Args...>* add(std::string const & name, 
                                                F* f, 
                                                EagleeyeRuntime fixed=EagleeyeRuntime(EAGLEEYE_UNKNOWN_RUNTIME)) {
    // 1.step check name exist
    if(nodes_map_.find(name) != nodes_map_.end()){
      EAGLEEYE_LOGE("node %s has been existed", name.c_str());
    }

    // 2.step build node
    auto * n = makeNode<F, Args...>(this, f, nodes_.size(), fixed);
    n->name = name;
    nodes_.push_back(n);
    if (name.size() > 0) { // TODO dupulicate check
      nodes_map_[name] = n;
    }
    if(f->getInputNum() == 0){
      entry_nodes_.push_back(n);
    }

    return n;
  }

  void bind(Node* from, int index, Node* to) {
    assert(!from->findNext(to));  
    assert(!to->findPrev(from));
    Edge * e = new Edge(*from, *to);

    from->next_.push_back(e);
    to->prev_.push_back(e);
    edges_.push_back(e);

    // assert(!from.find_data(to));  
    to->data_.push_back(from);
    to->index_.push_back(index);
  }

  void print(std::ostream & ost = std::cout, std::string post = "") {
    ost << "@" << std::endl;
    ost << "digraph { " << std::endl;
    ost << "  Node [" << std::endl;
    ost << "    shape = box" << std::endl;
    ost << "  ];" << std::endl;

    for (Node * n : nodes_) {
      ost << "  \"";
      if (n->name.size() > 0) {
        ost << n->name;
      } else {
        ost << n;
      }
      ost << "\" [" << std::endl;
      if (n->label.size() > 0) {
        ost << "  label = \"" << n->label << "\"" << std::endl;
      } else {
        ost << "  label = \"none\"" << std::endl;
      }
      ost << "  " << n->graphviz_node_property << std::endl;
      ost << "   ];" << std::endl;
    }

    for (Node * n : nodes_) {
      for (Edge * e : n->next_) {
        ost << "  \"";
        if (n->name.size() > 0) {
          ost << n->name;
        } else {
          ost << n;
        }
        ost << "\" -> \"";
        if (e->next().name.size() > 0) {
          ost << e->next().name;
        } else {
          ost << &(e->next());
        }
        ost << "\" [" << std::endl;
        ost << "    color = " << (e->is_locked() ? "red" : "green") << "" << std::endl;
        ost << "   ];" << std::endl;
      }
    }
    ost << graphviz_post << std::endl;
    ost << "}" << std::endl;
  }

  Node* findNode(std::string const & name) {
    auto it = nodes_map_.find(name);
    if (it == nodes_map_.end()) { return nullptr; }
    return it->second;
  }

  std::vector<Node*> getNodes(){
    return this->nodes_;
  }
  std::vector<Edge*> getEdges(){
    return this->edges_;
  }

  int size(){
    return this->nodes_.size();
  }

private:
  std::vector<Node*> nodes_;
  std::vector<Edge*> edges_;
  std::unordered_map<std::string, Node*> nodes_map_;
  std::vector<Node*> entry_nodes_;
  // std::map<std::string, TensorX> exit_nodes_;

  std::vector<Queue<Node*>> queue_;
  std::vector<worker> worker_;
  std::atomic_bool stop_;
  std::atomic_int waiting_stop_count_;  
  int exit_node_num_;
  Schedule* schedule_;
  std::string graphviz_post;

  void work(std::size_t id) {
    while (!stop_) {
      Node * n = nullptr;
      bool flag = false;
      for (std::size_t i = 0; i < numWorker(); ++i) {
        std::size_t index = (i + id) % numWorker();
        
        if (queue_[index].try_pop(n)) {
          flag = true;
          this->fire_(id, *n);
          break;
        }
      }

      if (!flag) {
        std::this_thread::sleep_for(std::chrono::milliseconds(128));
      }
    }
  }

  void fire_(std::size_t id, Node & n) {
    n.count_ = 0;
    // 1.step schedule 
    EagleeyeRuntime runtime = this->schedule_->getRuntime(&n);

    // 2.step run node op
    n.fire(runtime);
    EAGLEEYE_LOGD("finish %s on device %d", n.name.c_str(), int(runtime.type()));

    // 3.step finish node
    this->finish(&n);

    // 4.step active succeed nodes
    int i = id;
    for (Edge* next : n.next_) {
      unsigned n = next->next().count_.fetch_add(1);
      if (next->next().prev_.size() == (n + 1)){
        // 4.1.step transfer data asyn
        Node* next_node = &next->next();
        EagleeyeRuntime target_runtime = this->schedule_->getRuntime(next_node);
        next_node->transfer(target_runtime, true);
        EAGLEEYE_LOGD("transfer %s data to device %d", next_node->name.c_str(), int(target_runtime.type()));

        // 4.2.step push to queue, prepare to execute
        queue_[i % numWorker()].push(next_node);
        EAGLEEYE_LOGD("push %s to work queue %d", next_node->name.c_str(), i % numWorker());
        ++i;
      }
    }
  }

  /**
   * @brief finish node 
   * 
   * @param n 
   */
  void finish(Node* n){
    if(n->output_){
      waiting_stop_count_ += 1;
      void* temp = NULL;
      // no block
      n->fetch(temp, 0, false);
      n->output_ = false;
    }

    if(waiting_stop_count_ == exit_node_num_){
      stop_ = true;
    }
  }

  /**
   * @brief wating finish
   * 
   */
  void waitingUntilFinish() {
    for (auto & worker : worker_) {
      if (worker.thread.joinable()) {
        worker.thread.join();
      }
    }
  }
};
}
}
#endif
