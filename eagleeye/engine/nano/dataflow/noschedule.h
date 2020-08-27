#ifndef _EAGLEEYE_NOSCHEDULE_H_
#define _EAGLEEYE_NOSCHEDULE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeRuntime.h"
#include "eagleeye/common/EagleeyeTime.h"
#include "eagleeye/engine/nano/dataflow/schedule.h"
#include <vector>
#include <iostream>
namespace eagleeye{
namespace dataflow{
class NoSchedule:public Schedule{
public:
    NoSchedule(Graph* g, std::vector<EagleeyeRuntime> runtime)
        :Schedule(g, runtime){};
    virtual ~NoSchedule(){};

    virtual void collectStatistic(){
        EAGLEEYE_LOGD("No Collect DAG Run Statistic");
    };
    virtual void analyze(){
        EAGLEEYE_LOGD("No Analyze DAG");
    };
    virtual EagleeyeRuntime getRuntime(Node* node){
        if(node->getRuntime().type() != EAGLEEYE_UNKNOWN_RUNTIME){
            return node->getRuntime();
        }

        return EagleeyeRuntime(EAGLEEYE_CPU);
    };
};
}
}
#endif
