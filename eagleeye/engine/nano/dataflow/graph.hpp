#ifndef _EAGLEEYE_GRAPH_H_
#define _EAGLEEYE_GRAPH_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/engine/nano/dataflow/edge.hpp"
#include "eagleeye/engine/nano/dataflow/node.hpp"
#include "eagleeye/engine/nano/dataflow/node_impl.hpp"
#include "eagleeye/engine/nano/dataflow/queue.hpp"
#include "eagleeye/engine/nano/dataflow/worker.hpp"
#include "eagleeye/engine/nano/dataflow/schedule.h"
#include "eagleeye/engine/nano/dataflow/heft.h"
#include <atomic>
#include <iostream>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <vector>

namespace eagleeye{
namespace dataflow{
class Graph {
public:  
  std::string graphviz_post;
  Graph(std::vector<EagleeyeRuntime> runtimes, std::size_t num_worker = 4)
    :queue_(num_worker) {
    // 1.step build worker
    for (std::size_t i = 0; i < num_worker; ++i) {
      worker_.emplace_back(i);
    }

    // 2.step build resource schedule 
    schedule_ = new HEFT(this, runtimes);
  }

  virtual ~Graph () {
    for (Node * n : nodes_) {
      delete n;
    }
    for (Edge * e : edges_) {
      delete e;
    }

    delete schedule_;
  }

  void analyze(){
    // 1.step analyze compute resource
    this->schedule_->collect_statistic();
    this->schedule_->analyze();

    // 2.step do others
    for(Node* n: nodes_){
      EagleeyeRuntime d = this->schedule_->getRuntime(n);
      if(d.type() == EAGLEEYE_CPU){
        std::cout<<"node "<<n->getId()<<" on CPU"<<std::endl;
      }
      else{
        std::cout<<"node "<<n->getId()<<" on GPU"<<std::endl;
      }
    }
  }

  /**
   * @brief get worker thread number
   * 
   * @return std::size_t 
   */
  std::size_t num_worker () const noexcept {
    return worker_.size();
  }

  /**
   * @brief start worker thread
   * 
   */
  void run() {
    // 1.step reset running flag
    stop_ = false;
    std::unordered_map<std::string, bool>::iterator iter, iend(exit_nodes_map_.end());
    for(iter = exit_nodes_map_.begin(); iter != iend; ++iter){
      iter->second = false;
    }

    // 2.step start thread
    for (auto & worker : worker_) {
      worker.thread = std::thread([&](){ work(worker.id);});
    }

    // 3.step waiting finish
    waiting_until_finish();
  }

  /**
   * @brief node n is ok, start work thread
   */
  void run(Node & n) {
    fire(n);
    run();
  }

  /**
   * @brief node n is ready
   * 
   * @param n 
   */
  void fire(Node & n) {
    queue_[0].push(&n);
  }

  /**
   * @brief force to stop 
   * 
   */
  void stop () {
    stop_ = true;
    for (auto & worker : worker_) {
      if (worker.thread.joinable()) {
        worker.thread.join();
      }
    }
  }

  /**
   * @brief wating finish
   * 
   */
  void waiting_until_finish() {
    for (auto & worker : worker_) {
      if (worker.thread.joinable()) {
        worker.thread.join();
      }
    }
  }

  /**
   * @brief finish node 
   * 
   * @param n 
   */
  void finish(Node* n){
    if(n->getType() == EXIT){
      exit_nodes_map_[n->getName()] = true;
    }

    std::unordered_map<std::string, bool>::iterator iter, iend(exit_nodes_map_.end());
    int is_over = true;
    for(iter = exit_nodes_map_.begin(); iter != iend; ++iter){
      if(iter->second == false){
        is_over = false;
        break;
      }
    }
    if(is_over){
      stop_ = true;
    }
  }

  /**
   * @brief add computing node to graph
   * 
   * @tparam T 
   * @tparam F 
   * @tparam Args 
   * @param name 
   * @param f 
   * @param args 
   * @return deduce_node_type<F, T>& 
   */
  template <class T, class F, class ... Args>
  deduce_node_type<F, T, Args...> & add_node(std::string const & name, F && f,NodeType node_type=DEFAULT, EagleeyeRuntime fixed=EagleeyeRuntime(EAGLEEYE_UNKNOWN_RUNTIME)) {
    auto * n = make_node<T, F, Args...>(this, std::forward<F>(f), nodes_.size(), fixed);
    n->name = name;
    n->setType(node_type);
    nodes_.emplace(n);
    if (name.size() > 0) { // TODO dupulicate check
      nodes_map_[name] = n;
    }

    if(node_type == EXIT){
      exit_nodes_map_[name] = false;
    }
    return *n;
  }

  template <class F, class ... Args>
  deduce_node_type<F, void, Args...> & add_node(std::string const & name, F && f, NodeType node_type=DEFAULT, EagleeyeRuntime fixed=EagleeyeRuntime(EAGLEEYE_UNKNOWN_RUNTIME)) {
    auto * n = make_node<void, F, Args...>(this, std::forward<F>(f), nodes_.size(), fixed);
    n->name = name;
    n->setType(node_type);
    nodes_.emplace(n);
    if (name.size() > 0) { // TODO dupulicate check
      nodes_map_[name] = n;
    }
    if(node_type == EXIT){
      exit_nodes_map_[name] = false;
    }
    return *n;
  }

  void connect(Node & from, int index, Node & to) {
    assert(!from.find_next(to));  
    assert(!to.find_prev(from));
    Edge * e = new Edge(from, to);

    from.next_.push_back(e);
    to.prev_.push_back(e);
    edges_.emplace(e);

    // assert(!from.find_data(to));  
    to.data_.push_back(&from);
    to.index_.push_back(index);
  }

  // void connect_data(Node & from, Node & to) {
  //   // assert(!from.find_data(to));  
  //   from.data_.push_back(&to);
  // }

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

  Node* find_node(std::string const & name) {
    auto it = nodes_map_.find(name);
    if (it == nodes_map_.end()) { return nullptr; }
    return it->second;
  }

  std::unordered_set<Node*> getNodes(){
    return this->nodes_;
  }
  std::unordered_set<Edge*> getEdges(){
    return this->edges_;
  }

  int getNodesNum(){
    return this->nodes_.size();
  }

private:
  std::unordered_set<Node*> nodes_;
  std::unordered_set<Edge*> edges_;
  std::unordered_map<std::string, Node*> nodes_map_;

  std::vector<Queue<Node*>> queue_;
  std::vector<worker> worker_;
  std::atomic_bool stop_;
  std::unordered_map<std::string, bool> exit_nodes_map_;
  
  Schedule* schedule_;

  void work(std::size_t id) {
    while (!stop_) {
      Node * n = nullptr;
      bool flag = false;
      for (std::size_t i = 0; i < num_worker(); ++i) {
        std::size_t index = (i + id) % num_worker();
        
        if (queue_[index].try_pop(n)) {
          flag = true;
          fire_(id, *n);
          break;
        }
      }

      if (!flag) {
        std::this_thread::sleep_for(std::chrono::milliseconds(128));
      }
    }
  }

  void fire_ (std::size_t id, Node & n) {
    for (Edge * prev : n.prev_) {
      prev->unlock();
    }

    n.count_ = 0;
    // 1.step schedule 
    EagleeyeRuntime runtime = this->schedule_->getRuntime(&n);
    // 2.step run 
    n.fire(runtime);
    // 3.step finish
    this->finish(&n);

    int i = id;
    for (Edge* next : n.next_) {
      if (!next->lock()) {
        unsigned n = next->next().count_.fetch_add(1);
        if (n + 1 == next->next().prev_.size()) {
          queue_[i % num_worker()].push(&next->next());
          ++i;
        }
      }
    }
  }
};

// Node & operator>> (Node & left, Node & right) {
//   left.g()->connect(left, right);
//   left.g()->connect_data(right, left);
//   return right;
// }
}
}
#endif
