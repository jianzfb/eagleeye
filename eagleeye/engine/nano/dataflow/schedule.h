#ifndef _EAGLEEYE_SCHEDULE_H_
#define _EAGLEEYE_SCHEDULE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeRuntime.h"
#include <vector>

namespace eagleeye{
namespace dataflow{
class Graph;    
class Node;
class Schedule{
public:
    Schedule(Graph* g, std::vector<EagleeyeRuntime> runtime);
    virtual ~Schedule();
    
    /**
     * @brief Get the Runtime object for node
     * 
     * @param node 
     * @return EagleeyeRuntime 
     */
    virtual EagleeyeRuntime getRuntime(Node* node);

    /**
     * @brief analyze context
     * 
     */
    virtual void analyze() = 0;

    /**
     * @brief hardware statistics
     * 
     */
    virtual void collect_statistic() = 0;

protected:
    Graph* g_;              
    std::vector<EagleeyeRuntime> runtime_;  // 可用处理器集合
    int tasks_num_;         //  任务数量
    int processor_num_;     //  处理器数量
};   
}     
}
#endif