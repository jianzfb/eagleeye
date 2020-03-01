#include "eagleeye/engine/nano/dataflow/schedule.h"
#include "eagleeye/engine/nano/dataflow/graph.hpp"

namespace eagleeye{
namespace dataflow{
Schedule::Schedule(Graph* g, std::vector<EagleeyeRuntime> runtime)
    :g_(g),runtime_(runtime){
}   
Schedule::~Schedule(){

}

EagleeyeRuntime Schedule::getRuntime(Node* node){
    return EagleeyeRuntime(EAGLEEYE_CPU);    
}
}    
}