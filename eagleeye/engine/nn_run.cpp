#include "eagleeye/engine/nn_run.h"
namespace eagleeye
{
NNRun::NNRun(std::string model_name, 
            std::string device,
            std::vector<std::string> input_names,
            std::vector<std::vector<int64_t>> input_shapes,
            std::vector<std::string> output_names,
            std::vector<std::vector<int64_t>> output_shapes,
            int omp_num_threads, 
            int cpu_affinity_policy, 
            std::string writable_path){
    // this->m_engine = new ModelRun(model_name, 
    //                                 device,
    //                                 input_names,
    //                                 input_shapes,
    //                                 output_names,
    //                                 output_shapes,
    //                                 omp_num_threads,
    //                                 cpu_affinity_policy,
    //                                 writable_path);
    this->m_engine = NULL;
}

NNRun::~NNRun(){
    if(this->m_engine){
        delete m_engine;
    }
}

bool NNRun::run(std::map<std::string, unsigned char*> inputs, 
					 std::map<std::string, unsigned char*>& outputs){
    return m_engine->run(inputs, outputs);
}

bool NNRun::initialize(){
    return m_engine->initialize();
}

void NNRun::setWritablePath(std::string writable_path){
    m_engine->setWritablePath(writable_path);
}

std::string NNRun::getWritablePath(){
    return m_engine->getWritablePath();
}

void* NNRun::getInputPtr(std::string input_name){
    return m_engine->getInputPtr(input_name);
}

void NNRun::setOutputNameMap(std::map<std::string, std::string> name_map){
    m_engine->setOutputNameMap(name_map);
}

void NNRun::setOutputNameMap2(std::map<std::string, std::string> name_map){
    m_engine->setOutputNameMap2(name_map);
}

ModelEngine* NNRun::get(){
    return m_engine;
}
} // namespace eagleeye
