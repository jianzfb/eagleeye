#include "eagleeye/common/EagleeyeGraph.h"
#include "eagleeye/common/EagleeyeLog.h"

namespace eagleeye{

struct _NodeT{
    std::string name;
    int count;
    std::vector<_NodeT*> dependent_nodes;
};    

bool _LessSort(_NodeT* a,_NodeT* b) { 
    return (a->count < b->count); 
}

void deepSearch(_NodeT* node, int& sum){
    for(int i=0; i<node->dependent_nodes.size(); ++i){
        deepSearch(node->dependent_nodes[i], sum);
    }

    sum += node->count;
}

std::vector<std::string> eagleeye_topology_sort(std::map<std::string, std::vector<std::string>>& dependent_nodes){
    std::map<std::string, _NodeT*> nodes;
    std::map<std::string, std::vector<std::string>>::iterator iter, iend(dependent_nodes.end());
    // 1.step 初始化节点，并构建拓扑图
    for(iter = dependent_nodes.begin(); iter != iend; ++iter){
        if(nodes.find(iter->first) == nodes.end()){
            _NodeT* t = new _NodeT;
            t->name = iter->first;
            t->count = 1;
            nodes[iter->first] = t;
        }
        
        for(int i=0; i<iter->second.size(); ++i){
            if(nodes.find(iter->second[i]) == nodes.end()){
                _NodeT* t = new _NodeT;
                t->name = iter->second[i];
                t->count = 1;
                nodes[iter->second[i]] = t;
            }

            nodes[iter->first]->dependent_nodes.push_back(nodes[iter->second[i]]);
        }
    }

    // 2.step 根据依赖关系，节点排序
    std::map<std::string, int> record;
    for(iter = dependent_nodes.begin(); iter != iend; ++iter){
        int count = 0;
        for(int i=0; i<iter->second.size(); ++i){
            int sum = 0;
            deepSearch(nodes[iter->second[i]], sum);
            count += sum;
        }

        record[iter->first] = count + 1;
    }
    std::map<std::string, int>::iterator a,b(record.end());
    for(a = record.begin(); a != b; ++a){
        nodes[a->first]->count = a->second;
    }
    record.clear();

    for(iter = dependent_nodes.begin(); iter != iend; ++iter){
        int count = 0;
        for(int i=0; i<iter->second.size(); ++i){
            int sum = 0;
            deepSearch(nodes[iter->second[i]], sum);
            count += sum;
        }

        record[iter->first] = count + 1;
    }
    b = record.end();
    for(a=record.begin(); a!=b; ++a){
        nodes[a->first]->count = a->second;
    }
    
    std::vector<_NodeT*> node_list;
    std::map<std::string, _NodeT*>::iterator nn_iter, nn_iend(nodes.end());
    for(nn_iter = nodes.begin(); nn_iter != nn_iend; ++nn_iter){
        node_list.push_back(nn_iter->second);
    }

    sort(node_list.begin(),node_list.end(),_LessSort);//升序排列
    std::vector<std::string> result;
    for(int i=0; i<node_list.size(); ++i){
        result.push_back(node_list[i]->name);
    }
    for(nn_iter = nodes.begin(); nn_iter != nn_iend; ++nn_iter){
        delete nn_iter->second;
    }

    return result;
}
}