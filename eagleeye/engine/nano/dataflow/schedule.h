#ifndef _EAGLEEYE_SCHEDULE_H_
#define _EAGLEEYE_SCHEDULE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeRuntime.h"
#include "eagleeye/common/EagleeyeLog.h"
#include <vector>

namespace eagleeye{
namespace dataflow{
class Graph;    
class Node;
class Schedule{
public:
    Schedule(Graph* g, std::vector<EagleeyeRuntime> runtime)
        :g_(g),runtime_(runtime){};
    virtual ~Schedule(){};
    
    /**
     * @brief Get the Runtime object for node
     * 
     * @param node 
     * @return EagleeyeRuntime 
     */
    virtual EagleeyeRuntime getRuntime(Node* node){
        return EagleeyeRuntime(EAGLEEYE_CPU);    
    }

    /**
     * @brief analyze context
     * 
     */
    virtual void analyze() = 0;

    /**
     * @brief hardware statistics
     * 
     */
    virtual void collectStatistic() = 0;

protected:
    Graph* g_;              
    std::vector<EagleeyeRuntime> runtime_;  // 可用处理器集合
    int tasks_num_;         //  任务数量
    int processor_num_;     //  处理器数量
};   
}     
}
#endif