#ifndef _EAGLEEYE_NANO_ASYN_GRAPH_H_
#define _EAGLEEYE_NANO_ASYN_GRAPH_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/engine/nano/dataflow/edge.hpp"
#include "eagleeye/engine/nano/dataflow/node.hpp"
#include "eagleeye/engine/nano/dataflow/node_impl_asyn.hpp"
#include "eagleeye/engine/nano/dataflow/queue.hpp"
#include "eagleeye/engine/nano/dataflow/worker.hpp"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/dataflow/graph.hpp"

#include <atomic>
#include <iostream>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <vector>

namespace eagleeye{
namespace dataflow{
class AsynGraph{
public:
    AsynGraph(){
        // 仅支持固定设备运行，在创建节点是设置
    };
    virtual ~AsynGraph(){
        std::cout<<"in ~"<<std::endl;
        for (Node * n : m_nodes) {
            std::cout<<"delete "<<n->name<<std::endl;
            delete n;
        }
        for (Edge * e : m_edges) {
            delete e;
        }
    };

    void init(const char* model_path){
        // 1.step 模型加载
        if(model_path != NULL){
            // 从模型文件加载模型结构
            // 加载模型DAG，和参数
        }
        std::map<std::string, std::map<std::string, std::vector<float>>> params;
        this->init(params);
    }

    void init(std::map<std::string, std::map<std::string, std::vector<float>>> params){
        // 1.step 分析节点结构
        for(Node* n: m_nodes){
            if(n->getDependentNum() == 0){
                m_entry_nodes.push_back(n);
            }
        }

        // 2.step 初始化节点
        for(Node* n: m_nodes){
            std::map<std::string, std::vector<float>> node_param;
            std::map<std::string, std::map<std::string, std::vector<float>>>::iterator iter = params.find(n->name);
            if(iter != params.end()){
                node_param = iter->second;
            }
            n->init(EagleeyeRuntime(EAGLEEYE_CPU), node_param);
        }
    }

    void run(std::map<std::string, void*> inputs=std::map<std::string, void*>()){
        // 重置内部参数
        for(Node* n: m_nodes){
            n->count_ = 0;
            n->output_ = false;
        }

        // 驱动图运行
        Queue<std::pair<Node*,void*>> queue; 
        std::map<std::string, void*>::iterator in_iter, in_iend(inputs.end());
        for(auto & n : m_entry_nodes){
            in_iter = inputs.find(n->name);
            if(in_iter != in_iend){
                queue.push(std::pair<Node*,void*>(n, in_iter->second));
                continue;
            }
            queue.push(std::pair<Node*,void*>(n, NULL));
        }
        while(queue.size() > 0){
            std::pair<Node*,void*> data_and_value;
            if(!queue.try_pop(data_and_value)){
                break;
            }

            Node* node = data_and_value.first;
            void* value = data_and_value.second;

            // 激活节点运行
            node->fire(EagleeyeRuntime(EAGLEEYE_UNKNOWN_RUNTIME), value);

            // 分析依赖
            for (Edge* next: node->next_){
                // increment 1, for succeed node
                unsigned n = next->next().count_.fetch_add(1);

                // transfer data asynchronous
                Node* next_node = &next->next();
                // check, whether all dependents have been finish
                if (next->next().prev_.size() == (n + 1)){
                    queue.push(std::pair<Node*,void*>(next_node, NULL));
                }
            }
        }
    }

    void get(std::map<std::string, void*>& outputs){
        std::map<std::string, void*> result;
        std::map<std::string, void*>::iterator out_iter,out_iend(outputs.end());
        for(out_iter = outputs.begin(); out_iter != out_iend; ++out_iter){
            std::unordered_map<std::string, Node*>::iterator d_iter = m_nodes_map.find(out_iter->first);
            if(d_iter == m_nodes_map.end()){
                EAGLEEYE_LOGE("Output node %s dont exist.", out_iter->first.c_str());
                result[out_iter->first] = NULL;
                continue;
            }

            void* data = NULL;
            m_nodes_map[out_iter->first]->fetch(data, 0, true);
            result[out_iter->first] = data;
        }

        outputs = result;
    }

    void bind(Node* from, int from_i, Node* to, int to_i){
        assert(!from->findNext(to));  
        assert(!to->findPrev(from));
        Edge * e = new Edge(*from, from_i, *to, to_i);
        m_edges.push_back(e);

        from->next_.push_back(e);
        to->prev_.push_back(e);

        to->data_.push_back(from);
        to->index_.push_back(from_i);
        to->order_.push_back(to_i);
        if(to->inv_order_.size() <= to_i){
            to->inv_order_.resize(to_i+1);
        }
        to->inv_order_[to_i] = to->index_.size() - 1;
    }
    
    template <class F, class ... Args>
    deduce_asyn_node_type<F, Args...>* add(std::string name, 
                                        F f,
                                        int process_num, 
                                        EagleeyeRuntime fixed=EagleeyeRuntime(EAGLEEYE_UNKNOWN_RUNTIME)) {
        // 1.step assign name
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
        auto * n = makeAsynNode<F, Args...>(f, process_num, m_nodes.size(), fixed);
        n->name = name;
        m_nodes.push_back(n);
        m_nodes_map[name] = n;
        return n;
    }

protected:

private:
    std::vector<Node*>                        m_nodes;
    std::vector<Edge*>                        m_edges;
    std::unordered_map<std::string, Node*>    m_nodes_map;
    std::vector<Node*>                        m_entry_nodes;
};

} // namespace dataflow
} // namespace eagleeye


#endif