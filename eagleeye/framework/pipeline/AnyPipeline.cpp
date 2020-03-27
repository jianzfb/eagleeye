#include "eagleeye/framework/pipeline/AnyPipeline.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeVersion.h"
#include "eagleeye/common/EagleeyeFile.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include <string>


namespace eagleeye{
std::map<std::string, std::shared_ptr<AnyPipeline>> AnyPipeline::m_pipeline_map;
std::map<std::string, std::string> AnyPipeline::m_pipeline_version;
std::map<std::string, std::string> AnyPipeline::m_pipeline_signature;

AnyPipeline* AnyPipeline::getInstance(const char* pipeline_name){
    if(m_pipeline_map.find(std::string(pipeline_name)) == m_pipeline_map.end()){
        EAGLEEYE_LOGE("couldn't get pipeline %s's instance",pipeline_name);
        return NULL;
    }

    if(AnyPipeline::m_pipeline_map[std::string(pipeline_name)].get() == NULL){
        AnyPipeline::m_pipeline_map[std::string(pipeline_name)] = std::shared_ptr<AnyPipeline>(new AnyPipeline(pipeline_name));
    }

    return  AnyPipeline::m_pipeline_map[std::string(pipeline_name)].get();
}

void AnyPipeline::getRegistedPipelines(std::vector<std::string>& pipeline_names){
    std::map<std::string, std::shared_ptr<AnyPipeline>>::iterator iter, iend(AnyPipeline::m_pipeline_map.end());
    for(iter = AnyPipeline::m_pipeline_map.begin(); iter != iend; ++iter){
        pipeline_names.push_back(iter->first);
    }
}

bool AnyPipeline::registerPipeline(const char* pipeline_name, 
                                    const char* version, 
                                    const char* key){
    // 1.step check key
    // min_core_version@signature
    std::string key_str = key;
    std::string seperator_str = "@";
    std::vector<std::string> terms = split(key_str, seperator_str);
    std::string signature = terms[0];
    if(terms.size() >= 2){
        std::string min_core_version = terms[0];
        signature = terms[1];
        // 1.1.step check min core version
        bool is_ok = minimumVersionRequired(min_core_version);
        if(!is_ok){
            EAGLEEYE_LOGD("needed min core version is %s, now core version %s", min_core_version.c_str(), getVersion().c_str());
            return false;
        }
    }

    // 2.step create pipeline 
    m_pipeline_map[std::string(pipeline_name)] = std::shared_ptr<AnyPipeline>(new AnyPipeline(pipeline_name));
    m_pipeline_version[std::string(pipeline_name)] = std::string(version);
    m_pipeline_signature[std::string(pipeline_name)] = signature;

    return true;
} 

void AnyPipeline::releasePipeline(const char* pipeline_name){
    if(m_pipeline_map.find(std::string(pipeline_name)) == m_pipeline_map.end()){
        return;
    }
    m_pipeline_map[std::string(pipeline_name)] = std::shared_ptr<AnyPipeline>();
} 

AnyPipeline::AnyPipeline(const char* pipeline_name){
    this->m_is_initialize = false;
    this->m_init_func = NULL;
}   

AnyPipeline::~AnyPipeline(){
    std::map<std::string, AnyNode*>::iterator iter, iend(m_nodes.end());
    for(iter=this->m_nodes.begin(); iter!=iend; ++iter){
        iter->second->exit();
        delete iter->second;
    }
}

void AnyPipeline::setPipelineName(const char* name){
    this->m_name = name;
}

void AnyPipeline::getPipelineName(char* name){
    memcpy(name, this->m_name.c_str(), sizeof(char)*this->m_name.length());
}

const char* AnyPipeline::getPipelineVersion(){
    std::map<std::string, std::string>::iterator iter,iend(AnyPipeline::m_pipeline_version.end());
    for(iter = AnyPipeline::m_pipeline_version.begin(); iter!=iend; ++iter){
    }
    return AnyPipeline::m_pipeline_version[m_name].c_str();
}

bool AnyPipeline::start(const char* node_name){
    bool is_finish = true;
    if(node_name == NULL){
        // run whole pipeline
        std::map<std::string, AnyNode*>::iterator iter, iend(this->m_output_nodes.end());
        for(iter=this->m_output_nodes.begin(); iter!=iend; ++iter){
            iter->second->start();
            is_finish = is_finish & iter->second->isDataHasBeenUpdate();
        }
    }
    else{
        // run at ...
        this->m_nodes[std::string(node_name)]->start();
        is_finish = is_finish & this->m_nodes[std::string(node_name)]->isDataHasBeenUpdate();
    }

    return is_finish;
}

void AnyPipeline::isReady(int& is_ready){
    // all needed data is ok for pipeline
    std::map<std::string, AnyNode*>::iterator iter, iend(this->m_input_nodes.end());
    bool is_ok = true;
    for(iter=this->m_input_nodes.begin(); iter!=iend; ++iter){
        is_ok = is_ok & iter->second->selfcheck();
    }

    is_ready = is_ok ? 1 : 0;
}

void AnyPipeline::reset(){
    std::map<std::string, AnyNode*>::iterator iter, iend(this->m_output_nodes.end());
    for(iter=this->m_output_nodes.begin(); iter!=iend; ++iter){
        iter->second->reset();
    }
}

AnyNode* AnyPipeline::get(const char* node_name){
    if(this->m_nodes.find(std::string(node_name)) == this->m_nodes.end()){
        return NULL;
    }

    return this->m_nodes[std::string(node_name)];
}

bool AnyPipeline::add(AnyNode* node, const char* node_name, PipelineNodeType nodetype){
    if(this->m_nodes.find(std::string(node_name)) != this->m_nodes.end()){
        EAGLEEYE_LOGD("node %s has exist in pipeline", node);
        return false;
    }

    // set node name
    node->setUnitName(node_name);
    this->m_nodes[std::string(node_name)] = node;

    // 
    switch(nodetype){
        case SOURCE_NODE:
            this->m_input_nodes[std::string(node_name)] = node;
            break;
        case SINK_NODE:
            this->m_output_nodes[std::string(node_name)] = node;
            break;
        default:
            break;
    }
    return true;
}

void AnyPipeline::bind(const char* node_a, 
                       int port_a, 
                       const char* node_b, 
                       int port_b){
    if(this->m_nodes.find(std::string(node_a)) == this->m_nodes.end()){
        EAGLEEYE_LOGD("node a %s dont exist", node_a);
        return;
    }
    if(this->m_nodes.find(std::string(node_b)) == this->m_nodes.end()){
        EAGLEEYE_LOGD("node b %s dont exist", node_b);
        return;
    }

    AnyNode* node_a_ptr = this->m_nodes[std::string(node_a)];
    AnyNode* node_b_ptr = this->m_nodes[std::string(node_b)];
    if(node_b_ptr->getNumberOfInputSignals() < port_b+1){
        node_b_ptr->setNumberOfInputSignals(port_b+1);
    }
    node_b_ptr->setInputPort(node_a_ptr->getOutputPort(port_a), port_b);
}

void AnyPipeline::loadConfigure(std::string config_file){
    EAGLEEYE_LOGD("load configure file %s", config_file.c_str());
    // 1.step load structure of pipeline

    // 2.step load parameter of pipeline
    FILE* fp = fopen(config_file.c_str(), "rb");
    if(fp == NULL){
        EAGLEEYE_LOGD("configure file %s donot exist", config_file.c_str());
        return;
    }

    // 2.1.step load pipeline name
    int name_size;
    fread(&name_size, 1, sizeof(int), fp);
    char* temp_mem = (char*)malloc(name_size);
    fread(temp_mem, 1, name_size, fp);
    if(this->m_name != std::string(temp_mem)){
        EAGLEEYE_LOGD("pipeline configure file not match %s", this->m_name.c_str());
        return;
    }
    free(temp_mem);
    EAGLEEYE_LOGD("load pipeline %s from configrue file", this->m_name.c_str());

    // 2.2.step load pipeline version
    int version_size;
    fread(&version_size, 1, sizeof(int), fp);
    temp_mem = (char*)malloc(version_size);
    fread(temp_mem, 1, version_size, fp);
    if(this->m_version != std::string(temp_mem)){
        EAGLEEYE_LOGD("version not match (%s <- %s)", this->m_version.c_str(), temp_mem);
        return;
    }
    free(temp_mem);

    // 2.3.step load pipeline signature
    int signature_size;
    fread(&signature_size, 1, sizeof(int), fp);
    temp_mem = (char*)malloc(signature_size);
    fread(temp_mem, 1, signature_size, fp);
    // pass
    free(temp_mem);

    // 2.4.step load node params from configure file
    int node_num;
    fread(&node_num, 1, sizeof(int), fp);
    EAGLEEYE_LOGD("node number %d in config file", node_num);
    std::map<std::string, std::shared_ptr<char>> nodes_config;
    for(int i=0; i<node_num; ++i){
        int node_size;
        fread(&node_size, 1, sizeof(int), fp);

        char* node_mem = (char*)malloc(node_size);
        fread(node_mem, 1, node_size, fp);
        
        char* node_mem_alias = node_mem;
        int t = *((int*)node_mem_alias);
        node_mem_alias += sizeof(int);

        int node_name_size = *((int*)node_mem_alias);
        char* node_name = node_mem_alias + sizeof(int);

        EAGLEEYE_LOGD("node %s param block size %d", node_name, node_size);
        nodes_config[std::string(node_name)] = std::shared_ptr<char>(node_mem, [](char* data){free(data);});
    }
    fclose(fp);

    std::map<std::string, AnyNode*>::iterator iter, iend(this->m_output_nodes.end());
    for(iter=this->m_output_nodes.begin(); iter!=iend; ++iter){
        iter->second->loadConfigure(nodes_config);
    }
}
void AnyPipeline::saveConfigure(std::string config_file){
    EAGLEEYE_LOGD("analyze pipeline %s configure", this->m_name.c_str());
    std::map<std::string, std::shared_ptr<char>> nodes_config;
    std::map<std::string, AnyNode*>::iterator iter, iend(this->m_output_nodes.end());
    for(iter=this->m_output_nodes.begin(); iter!=iend; ++iter){
        iter->second->saveConfigure(nodes_config);
    }

    EAGLEEYE_LOGD("write pipeline %s configure to file", this->m_name.c_str());
    FILE* fp = fopen(config_file.c_str(),"wb");
    if(fp == NULL){
        EAGLEEYE_LOGD("couldnt build file");
        return;
    }
    // 1.step save structure of pipeline

    // 2.step save parameter of pipeline
    // 2.1.step save pipeline name
    int name_size = this->m_name.size() + 1;
    fwrite(&name_size, 1, sizeof(int), fp);
    char* temp_mem = (char*)malloc(name_size);
    memset(temp_mem, '\0', name_size);
    memcpy(temp_mem, this->m_name.c_str(), this->m_name.size());
    fwrite(temp_mem, 1, name_size, fp);
    free(temp_mem);
    
    // 2.2.step save pipeline version
    int version_size = this->m_version.size() + 1;
    fwrite(&version_size, 1, sizeof(int), fp);
    temp_mem = (char*)malloc(version_size);
    memset(temp_mem, '\0', version_size);
    memcpy(temp_mem, this->m_version.c_str(), this->m_version.size());
    fwrite(temp_mem, 1, version_size, fp);
    free(temp_mem);

    // 2.3.step save pipeline signature
    int signature_size = this->m_signature.size() + 1;
    fwrite(&signature_size, 1, sizeof(int), fp);
    temp_mem = (char*)malloc(signature_size);
    memset(temp_mem, '\0', signature_size);
    memcpy(temp_mem, this->m_signature.c_str(), this->m_signature.size());
    fwrite(temp_mem, 1, signature_size, fp);
    free(temp_mem);

    // 2.4.step save pipeline nodes configure
    int nodes_num = nodes_config.size();
    fwrite(&nodes_num, 1, sizeof(int), fp);

    std::map<std::string, std::shared_ptr<char>>::iterator nc_iter, nc_iend(nodes_config.end());
    for(nc_iter=nodes_config.begin(); nc_iter != nc_iend; ++nc_iter){
        char* data = nc_iter->second.get();
        int size = *((int*)data);
        // size
        fwrite(&size, 1, sizeof(int), fp);
        // body
        fwrite(data, 1, size, fp);
    }
    fclose(fp);
}

void AnyPipeline::setParameter(const char* node_name, 
                               const char* param_name, 
                               const void* value){
    if(node_name == NULL || strcmp(node_name, "") == 0){
        EAGLEEYE_LOGE("node name is empty");
        return;
    }
    if(param_name == NULL || strcmp(param_name, "") == 0){
        EAGLEEYE_LOGE("param name is empty");
        return;
    }
    
    // node_name/{}/param_name
    bool issuccess=false;
    std::map<std::string, AnyMonitor*>::iterator iter, iend(this->m_monitor_params.end());
    for(iter = this->m_monitor_params.begin(); iter != iend; ++iter){
        if(startswith(iter->first, node_name) && endswith(iter->first, param_name)){
            iter->second->setVar(value);
            issuccess = true;
        }
    }
    if(!issuccess){
        EAGLEEYE_LOGE("%s/%s not in monitor parameters", node_name, param_name);
    }

    // std::string param_key = std::string(node_name) + std::string("/") + std::string(param_name);
    // if(this->m_monitor_params.find(param_key) == this->m_monitor_params.end()){
    //     EAGLEEYE_LOGE("%s/%s not in monitor parameters", node_name, param_name);
    //     return;
    // }
    // this->m_monitor_params[param_key]->setVar(value);
}

void AnyPipeline::getParameter(const char* node_name, 
                               const char* param_name,
                               void* value){
    if(node_name == NULL || strcmp(node_name, "") == 0){
        EAGLEEYE_LOGE("node name is empty");
        return;
    }
    if(param_name == NULL || strcmp(param_name, "") == 0){
        EAGLEEYE_LOGE("param name is empty");
        return;
    }

    // node_name/{}/param_name
    std::map<std::string, AnyMonitor*>::iterator iter, iend(this->m_monitor_params.end());
    for(iter = this->m_monitor_params.begin(); iter != iend; ++iter){
        if(startswith(iter->first, node_name) && endswith(iter->first, param_name)){
            iter->second->getVar(value);
            return;
        }
    }

    EAGLEEYE_LOGE("%s/%s not in monitor parameters", node_name, param_name);

    // std::string param_key = std::string(node_name) + std::string("/") + std::string(param_name);
    // if(this->m_monitor_params.find(param_key) == this->m_monitor_params.end()){
    //     EAGLEEYE_LOGE("%s/%s not in monitor parameters", node_name, param_name);
    //     return;
    // }
    // this->m_monitor_params[param_key]->getVar(value);
}

void AnyPipeline::setInput(const char* node_name, 
                           void* data, 
                           const int* data_size, 
                           const int data_dims,
                           const int data_type){
    if(node_name == NULL || strcmp(node_name, "") == 0){
        EAGLEEYE_LOGE("node name is empty");
        return;
    }

    std::string input_key = std::string(node_name);    
    int port = 0;
    if(input_key.find("/") != std::string::npos){
        std::vector<std::string> kterms = split(input_key, "/");
        input_key = kterms[0];
        port = tof<int>(kterms[1]);
    }

    if(this->m_input_nodes.find(input_key) == this->m_input_nodes.end()){
        EAGLEEYE_LOGE("%s is not input node", node_name);
        return;
    }
    
    if(data_size[0] == 0 || data_size[1] == 0 || data_size[2] == 0){
        EAGLEEYE_LOGE("data size abnormal %d %d %d", data_size[0], data_size[1], data_size[2]);
        return;
    }

    if(data_type != this->m_input_nodes[input_key]->getOutputPort(port)->getSignalValueType()){
        EAGLEEYE_LOGE("data type %d not equal to input %d", data_type, int(this->m_input_nodes[input_key]->getOutputPort(port)->getSignalValueType()));
        return;
    }

    this->m_input_nodes[input_key]->getOutputPort(port)->setSignalContent(data, data_size, data_dims);
    this->m_input_nodes[input_key]->modified();
}

void AnyPipeline::getOutput(const char* node_name, 
                            void*& data, 
                            int* data_size, 
                            int& data_dims,
                            int& data_type){
    if(node_name == NULL || strcmp(node_name, "") == 0){
        EAGLEEYE_LOGE("node name is empty");
        return;
    }

    std::string output_key = std::string(node_name);
    int port = 0;
    if(output_key.find("/") != std::string::npos){
        std::vector<std::string> kterms = split(output_key, "/");
        output_key = kterms[0];
        port = tof<int>(kterms[1]);
    }

    if(this->m_output_nodes.find(output_key) == this->m_output_nodes.end()){
        EAGLEEYE_LOGE("%s is not output node", node_name);
        return;
    }
    
    this->m_output_nodes[output_key]->getOutputPort(port)->getSignalContent(data, data_size, data_dims, data_type);
}

void AnyPipeline::getNodeOutput(const char* node_name, void*& data, int* data_size, int& data_dims, int& data_type){
    if(node_name == NULL || strcmp(node_name, "") == 0){
        EAGLEEYE_LOGE("node name is empty");
        return;
    }
    
    std::string output_key = std::string(node_name);
    int port = 0;
    if(output_key.find("/") != std::string::npos){
        std::vector<std::string> kterms = split(output_key, "/");
        output_key = kterms[0];
        port = tof<int>(kterms[1]);
    }

    if(this->m_nodes.find(output_key) == this->m_nodes.end()){
        EAGLEEYE_LOGE("%s is not output node", node_name);
        return;
    }
    
    this->m_nodes[output_key]->getOutputPort(port)->getSignalContent(data, data_size, data_dims, data_type);
}


void AnyPipeline::getPipelineInputs(std::vector<std::string>& input_nodes, 
                                    std::vector<std::string>& input_types, 
                                    std::vector<std::string>& input_sources){
    // input_key - type - source
    std::map<std::string, AnyNode*>::iterator iter,iend(this->m_input_nodes.end());
    for(iter = this->m_input_nodes.begin(); iter != iend; ++iter){
        input_nodes.push_back(iter->first);
        input_types.push_back(iter->second->getOutputPort(0)->getSignalType());
        input_sources.push_back(iter->second->getOutputPort(0)->getSignalTarget());
    }
}

void AnyPipeline::getPipelineOutputs(std::vector<std::string>& output_nodes,
                                     std::vector<std::string>& output_types,
                                     std::vector<std::string>& output_targets){
    // output_key - type - target
    std::map<std::string, AnyNode*>::iterator iter, iend(this->m_output_nodes.end());
    for(iter = this->m_output_nodes.begin(); iter != iend; ++iter){
        output_nodes.push_back(iter->first);
        std::string signal_type = "";
        std::string signal_target = "";
        for(int index = 0; index<iter->second->getNumberOfOutputSignals(); ++index){
            if(index != iter->second->getNumberOfOutputSignals() - 1){
                signal_type += std::string(iter->second->getOutputPort(index)->getSignalType()) + "/";
                signal_target += std::string(iter->second->getOutputPort(index)->getSignalTarget()) + "/";
            }
            else{
                signal_type += std::string(iter->second->getOutputPort(index)->getSignalType());
                signal_target += std::string(iter->second->getOutputPort(index)->getSignalTarget());
            }
        }

        output_types.push_back(signal_type);
        output_targets.push_back(signal_target);
    }
}

void AnyPipeline::getPipelineMonitors(std::vector<std::string>& monitor_names,
                                      std::vector<int>& monitor_types,
                                      std::vector<std::string>& monitor_range){
    std::map<std::string, AnyMonitor*>::iterator iter, iend(this->m_monitor_params.end());
    for(iter = this->m_monitor_params.begin(); iter != iend; ++iter){
        // format 1: node_name/param_name
        // format 2: node_name/{}/param_name
        std::vector<std::string> terms = split(iter->first, "/");
        std::string monitor_name;
        if(terms.size() == 2){
            // format 1
            monitor_name = iter->first;
        }
        else{
            // format 2
            monitor_name = terms[0]+"/"+terms[2];
        }

        monitor_names.push_back(monitor_name);
        monitor_types.push_back(int(iter->second->monitor_var_type));
        monitor_range.push_back(std::string(iter->second->getMin())+"/"+std::string(iter->second->getMax()));
    }
}

void AnyPipeline::initialize(const char* configure_folder){
    if(this->m_is_initialize){
        EAGLEEYE_LOGD("pipeline %s has been initialized");
        return;
    }
    // 设置基本信息
    if(AnyPipeline::m_pipeline_version.find(this->m_name) == AnyPipeline::m_pipeline_version.end()){
        EAGLEEYE_LOGD("pipeline %s version not be register", this->m_name.c_str());
        return;
    }    
    this->m_version = AnyPipeline::m_pipeline_version[this->m_name];
    if(AnyPipeline::m_pipeline_signature.find(this->m_name) == AnyPipeline::m_pipeline_signature.end()){
        EAGLEEYE_LOGD("pipeline %s signature not be register", this->m_name.c_str());
        return;
    }
    this->m_signature = AnyPipeline::m_pipeline_signature[this->m_name];
    EAGLEEYE_LOGD("pipeline %s basic information", this->m_name.c_str());
    EAGLEEYE_LOGD("version      %s", this->m_version.c_str());
    EAGLEEYE_LOGD("signature    %s", this->m_signature.c_str());

    // 初始化管道
    EAGLEEYE_LOGD("build pipeline %s structure", this->m_name.c_str());
    m_init_func();
    
    // 分析连接关系
    EAGLEEYE_LOGD("analyze pipeline %s structure", this->m_name.c_str());
    // finding output nodes
    // this->m_output_nodes.clear();
    // std::map<std::string, bool>::iterator iter, iend(this->m_is_output_nodes.end());
    // for(iter = this->m_is_output_nodes.begin(); iter != iend; ++iter){
    //     if(iter->second){
    //         this->m_output_nodes[iter->first] = this->m_nodes[iter->first];
    //     }
    // }
    EAGLEEYE_LOGD("%s has %d output nodes", this->m_name.c_str(), this->m_output_nodes.size());
    std::map<std::string, AnyNode*>::iterator output_iter, output_iend(this->m_output_nodes.end());
    for(output_iter=this->m_output_nodes.begin(); output_iter != output_iend; ++output_iter){
        EAGLEEYE_LOGD("output node name %s", output_iter->first.c_str());
    }
    if(m_output_nodes.size() == 0){
        EAGLEEYE_LOGE("dont have output node in pipeline");
        return;
    }

    // get all monitors
    this->m_monitor_params.clear();
    std::map<std::string,std::vector<AnyMonitor*>> pipeline_monitors;
    std::map<std::string, AnyNode*>::iterator output_node_iter, output_node_iend(this->m_output_nodes.end());
    for(output_node_iter=this->m_output_nodes.begin(); output_node_iter!=output_node_iend; ++output_node_iter){
        output_node_iter->second->getPipelineMonitors(pipeline_monitors);
    }

    std::map<std::string,std::vector<AnyMonitor*>>::iterator monitor_iter, monitor_iend(pipeline_monitors.end());
    for(monitor_iter=pipeline_monitors.begin(); monitor_iter!=monitor_iend; ++monitor_iter){
        std::string node_name = monitor_iter->first;

        for(int i=0; i<monitor_iter->second.size(); ++i){
            std::string param_name = monitor_iter->second[i]->monitor_var_text;
            std::string param_key = std::string(node_name) + std::string("/") + std::string(param_name);

            this->m_monitor_params[param_key] = monitor_iter->second[i];
        }
    }

    // get all input signals
    // this->m_input_nodes.clear();
    // for(output_node_iter=this->m_output_nodes.begin(); output_node_iter!=output_node_iend; ++output_node_iter){
    //     output_node_iter->second->getPipelineInputs(this->m_input_nodes);
    // }

    EAGLEEYE_LOGD("%s has %d input nodes", this->m_name.c_str(), this->m_input_nodes.size());
    std::map<std::string,AnyNode*>::iterator input_iter, input_iend(this->m_input_nodes.end());
    for(input_iter=this->m_input_nodes.begin(); input_iter!=input_iend; ++input_iter){
        EAGLEEYE_LOGD("input node name %s", input_iter->first.c_str());
    }

    if(configure_folder != NULL){
        EAGLEEYE_LOGD("try load config file %s", this->m_name.c_str());
        std::string configure_file_path = std::string(configure_folder) + this->m_name + ".pipeline";
        if(isfileexist(configure_file_path.c_str())){
            this->loadConfigure(configure_file_path);
            EAGLEEYE_LOGD("finish config file loading");
        }
        else{
            EAGLEEYE_LOGD("config file not exist in %s", configure_folder);
        }
    }
}

void AnyPipeline::addFeadbackRule(const char* trigger_node, int trigger_node_state, const char* response_node, const char* response_action){
    this->get(response_node)->addFeadbackRule(trigger_node, trigger_node_state, response_action);
}

void AnyPipeline::setInitFunc(INITIALIZE_PLUGIN_PIPELINE_FUNC func){
    m_init_func = func;
}
}