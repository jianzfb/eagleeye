#include "eagleeye/framework/pipeline/AnyPipeline.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeVersion.h"
#include "eagleeye/common/EagleeyeFile.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/AnyRegister.h"
#include "eagleeye/processnode/CopyNode.h"
#include "eagleeye/processnode/Placeholder.h"
#include "eagleeye/common/EagleeyeIni.h"
#include "eagleeye/common/EagleeyeGraph.h"
#include "eagleeye/processnode/GroupNode.h"
#ifdef EAGLEEYE_OPENGL
#include "eagleeye/common/EagleeyeShader.h"
#endif
#include <string>


namespace eagleeye{
std::map<std::string, std::shared_ptr<AnyPipeline>> AnyPipeline::m_pipeline_map;
std::map<std::string, std::string> AnyPipeline::m_pipeline_version;
std::map<std::string, std::string> AnyPipeline::m_pipeline_signature;
std::string AnyPipeline::m_plugin_root;
std::shared_ptr<RenderContext> AnyPipeline::m_render_context;

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

bool AnyPipeline::isExist(const char* pipeline_name){
    if(m_pipeline_map.find(std::string(pipeline_name)) == m_pipeline_map.end()){
        return false;
    }

    return true;
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
    // TODO: 加入认证环节
    // 基于管线名称+version,和key进行验证
    // 1.step 加载部署文件，并获得插件基本配置信息
    // 基于key获得，管线名称，版本，有效期
    std::string deploy_file = AnyPipeline::m_plugin_root+"/"+pipeline_name+"/"+pipeline_name+".ini";
    bool is_approve = false;
    std::string ID = "";                        // 从服务端注册获得
    std::string DEPENDENT_PIPELINE = "";        // 依赖管线和导入端口名字(PIPELINE:FROM-NODE:TO-NODE;PIPELINE:FROM-NODE:TO-NODE)
    std::string INPUT = "";                     // 插件输入名字（数据源:节点名字;数据源:节点名字;）
    std::string OUTPUT = "";                    // 插件输出名字（输出目标:节点名字;输出目标:节点名字;）

    if(isfileexist(deploy_file.c_str())){
        // 1.1.step 获得关键信息
        EagleeyeINI ini_reader;
        ini_reader.readINI(deploy_file);
        
        // APP层将根据ID，获取插件依赖管线信息，并重写初始化配置文件
        ID = ini_reader.getValue("BASE", "ID");
        DEPENDENT_PIPELINE = ini_reader.getValue("BASE", "DEPENDENT_PIPELINE");
        INPUT = ini_reader.getValue("BASE", "INPUT");
        OUTPUT = ini_reader.getValue("BASE", "OUTPUT");

        EAGLEEYE_LOGI("pipeline ID %s", ID.c_str());
        EAGLEEYE_LOGI("pipeline DEPENDENT %s", DEPENDENT_PIPELINE.c_str());
        EAGLEEYE_LOGI("pipeline INPUT %s", INPUT.c_str());
        EAGLEEYE_LOGI("pipeline OUTPUT %s", OUTPUT.c_str());

        // 1.2.step 检查授权
        if(ID != key){
            EAGLEEYE_LOGE("plugin not approved, check ID and key");
            return false;
        }
        
        // 授权插件
        is_approve = true;
    }
    else{
        // 未授权插件
        // 未授权插件，无法进行插件使用数据统计以及费用记录（一般用于个人开始使用）
        is_approve = false;
    }

    // 2.step create pipeline 
    m_pipeline_map[std::string(pipeline_name)] = std::shared_ptr<AnyPipeline>(new AnyPipeline(pipeline_name));
    m_pipeline_version[std::string(pipeline_name)] = std::string(version);
    m_pipeline_signature[std::string(pipeline_name)] = key;

    // 设置授权信息
    m_pipeline_map[std::string(pipeline_name)]->approve(is_approve);

    // 设置插件依赖信息
    if(!DEPENDENT_PIPELINE.empty()){
        m_pipeline_map[std::string(pipeline_name)]->setDependentPipelines(DEPENDENT_PIPELINE);
    }
    return true;
} 

bool AnyPipeline::registerSignal(const char* pipeline_name, const char* node_name){
    if(m_pipeline_map.find(std::string(pipeline_name)) == m_pipeline_map.end()){
        EAGLEEYE_LOGE("Pipeline name %s dont exist.", pipeline_name);
        return false;
    }

    AnyNode* node = m_pipeline_map[std::string(pipeline_name)]->get(node_name);
    if(node == NULL){
        EAGLEEYE_LOGE("Node %s dont exist in pipeline %s.", node_name, pipeline_name);
        return false;
    }

    std::string signal_name = formatString("%s/%s", pipeline_name, node_name);
    std::string input_key = std::string(node_name);    
    int port = 0;
    if(input_key.find("/") != std::string::npos){
        std::vector<std::string> kterms = split(input_key, "/");
        input_key = kterms[0];
        port = tof<int>(kterms[1]);
    }
    if(port >= node->getNumberOfOutputSignals()){
        EAGLEEYE_LOGE("Port %d not in Node %s output in pipeline %s.", port, node_name, pipeline_name);
        return false;
    }

    bool state = AnyRegister::get()->registerSignal(signal_name,  node->getOutputPort(port));
    return state;
}

bool AnyPipeline::checkSignalMatching(const char* pipeline_name, const char* node_name, const char* register_node_name){
    if(m_pipeline_map.find(std::string(pipeline_name)) == m_pipeline_map.end()){
        EAGLEEYE_LOGE("Pipeline name %s dont exist.", pipeline_name);
        return false;
    }

    AnyNode* node = m_pipeline_map[std::string(pipeline_name)]->get(node_name);
    if(node == NULL){
        EAGLEEYE_LOGE("Node %s dont exist in pipeline %s.", node_name, pipeline_name);
        return false;
    }

    std::string input_key = std::string(node_name);    
    int port = 0;
    if(input_key.find("/") != std::string::npos){
        std::vector<std::string> kterms = split(input_key, "/");
        input_key = kterms[0];
        port = tof<int>(kterms[1]);
    }
    if(port >= node->getNumberOfOutputSignals()){
        EAGLEEYE_LOGE("Port %d not in Node %s output in pipeline %s.", port, node_name, pipeline_name);
        return false;
    }

    AnySignal* register_sig = AnyRegister::get()->signal(register_node_name);
    if(register_sig == NULL){
        return false;
    }

    if(register_sig->getSignalType() != node->getOutputPort(port)->getSignalType()){
        return false;
    }

    return true;
}

void AnyPipeline::releasePipeline(const char* pipeline_name){
    if(m_pipeline_map.find(std::string(pipeline_name)) == m_pipeline_map.end()){
        return;
    }

    // 清除管线注册
    m_pipeline_map.erase(m_pipeline_map.find(std::string(pipeline_name)));
} 

void AnyPipeline::approve(bool approve){
    this->m_is_approved = approve;
}

void AnyPipeline::setDependentPipelines(std::string info){
    std::string separator = ";";
    std::vector<std::string> kkvv = split(info, separator);
    for(int i=0; i<kkvv.size(); ++i){
        std::string s = ":";
        std::vector<std::string> pipeline_from_to = split(kkvv[i], s);
        std::string pipeline_name = pipeline_from_to[0];
        std::string from_node = pipeline_from_to[1];
        std::string to_node = pipeline_from_to[2];

        m_dependent_pipelines[pipeline_name].push_back(std::make_pair(from_node, to_node));
    }
}

AnyPipeline::AnyPipeline(const char* pipeline_name){
    this->m_init_func = NULL;
}   

AnyPipeline::~AnyPipeline(){
    // notify exit
    std::map<std::string, AnyNode*>::iterator output_node_iter, output_node_iend(this->m_output_nodes.end());
    for(output_node_iter=this->m_output_nodes.begin(); output_node_iter!=output_node_iend; ++output_node_iter){
        output_node_iter->second->exit();
    }

    std::map<std::string, AnyNode*>::iterator iter, iend(m_nodes.end());
    for(iter=this->m_nodes.begin(); iter!=iend; ++iter){
        delete iter->second;
    }

    std::vector<AnyNode*>::iterator placeholder_iter, placeholder_iend(m_using_placeholders.end());
    for(placeholder_iter=m_using_placeholders.begin(); placeholder_iter!=placeholder_iend; ++placeholder_iter){
        delete *placeholder_iter;
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

void AnyPipeline::refresh(){
    std::map<std::string, AnyNode*>::iterator iter, iend(this->m_nodes.end());
    for(iter = this->m_nodes.begin(); iter != iend; ++iter){
        if(iter->second->getNodeCategory() == RENDER){
            iter->second->modified();
        }
    }
}

bool AnyPipeline::start(const char* node_name, const char* ignore_prefix){
    EAGLEEYE_LOGD("Pipeline %s start", this->m_name.c_str());

    bool is_finish = true;
    if(node_name == NULL){
        // run whole pipeline
        for(int i=0; i<this->m_order_nodes.size(); ++i){
            std::string order_name = m_order_nodes[i];
            bool is_ignore = false;
            if(ignore_prefix != NULL && (ignore_prefix[0] != '\0')){
                std::string ignore_prefix_str = ignore_prefix;
                std::string separator = ",";
                std::vector<std::string> terms = split(ignore_prefix_str, separator);
                for(int t=0; t<terms.size(); ++t){
                    if(startswith(order_name, terms[t])){
                        is_ignore = true;
                        break;
                    }
                }
            }
            if(is_ignore){
                EAGLEEYE_LOGD("Ignore %s.", order_name.c_str());
                continue;
            }

            is_finish = is_finish & m_output_nodes[order_name]->start();
            // is_finish = is_finish & m_output_nodes[order_name]->isDataHasBeenUpdate();  
        }
    }
    else{
        // run at ...
        if(this->m_nodes.find(std::string(node_name)) == this->m_nodes.end()){
            return false;
        }

        is_finish = is_finish & this->m_nodes[std::string(node_name)]->start();
        // is_finish = is_finish & this->m_nodes[std::string(node_name)]->isDataHasBeenUpdate();
    }

    return is_finish;
}

void AnyPipeline::wait(const char* node_name, const char* ignore_prefix){
    EAGLEEYE_LOGD("Pipeline %s wait", this->m_name.c_str());

    if(node_name == NULL){
        // run whole pipeline
        std::map<std::string, AnyNode*>::iterator iter, iend(this->m_output_nodes.end());
        for(iter=this->m_output_nodes.begin(); iter!=iend; ++iter){
            if(ignore_prefix != NULL && (ignore_prefix[0] != '\0')){
                if(startswith(iter->first, ignore_prefix)){
                    EAGLEEYE_LOGD("Ignore %s.", iter->first.c_str());
                    continue;
                }
            }

            iter->second->wait();
        }
    }
    else{
        // run at ...
        if(this->m_nodes.find(std::string(node_name)) == this->m_nodes.end()){
            return;
        }

        this->m_nodes[std::string(node_name)]->wait();
    }
}

void AnyPipeline::reset(){
    std::map<std::string, AnyNode*>::iterator iter, iend(this->m_output_nodes.end());
    for(iter=this->m_output_nodes.begin(); iter!=iend; ++iter){
        iter->second->preset();
    }
}

AnyNode* AnyPipeline::get(const char* node_name){
    if(this->m_nodes.find(std::string(node_name)) == this->m_nodes.end()){
        return NULL;
    }

    return this->m_nodes[std::string(node_name)];
}

bool AnyPipeline::add(AnyNode* node, const char* node_name){
    if(this->m_nodes.find(std::string(node_name)) != this->m_nodes.end()){
        EAGLEEYE_LOGD("node %s has exist in pipeline", node);
        return false;
    }

    // set node name
    node->setUnitName(node_name);
    // set pipeline
    node->setPipeline(this);
    this->m_nodes[std::string(node_name)] = node;
    return true;
}

void AnyPipeline::bind(const char* node_a, 
                       int port_a, 
                       const char* node_b, 
                       int port_b,
                       bool is_recurrent){
    if(this->m_nodes.find(std::string(node_a)) == this->m_nodes.end()){
        EAGLEEYE_LOGD("Node a %s dont exist.", node_a);
        return;
    }
    if(this->m_nodes.find(std::string(node_b)) == this->m_nodes.end()){
        EAGLEEYE_LOGD("Node b %s dont exist.", node_b);
        return;
    }

    AnyNode* node_a_ptr = this->m_nodes[std::string(node_a)];
    AnyNode* node_b_ptr = this->m_nodes[std::string(node_b)];
    if(node_b_ptr->getNumberOfInputSignals() < port_b+1){
        node_b_ptr->setNumberOfInputSignals(port_b+1);
    }
    AnySignal* output_sig = NULL;
    if(is_recurrent){
        output_sig = node_a_ptr->getRecurrentOutputPort(port_a);
    }
    else{
        output_sig = node_a_ptr->getOutputPort(port_a);
    }
    if(output_sig == NULL){
        EAGLEEYE_LOGD("Node a output signal is NULL");
    }

    node_b_ptr->setInputPort(output_sig, port_b);
}

std::string AnyPipeline::group(std::vector<std::string> group_nodes, const char* node_name){
    for(int i=0; i<group_nodes.size(); ++i){
        std::string nn = group_nodes[i];
        //  检查node是否存在
        if(this->m_nodes.find(nn) == this->m_nodes.end()){
            EAGLEEYE_LOGD("Node %s dont exist", nn.c_str());
            return "";
        }
    }

    if(this->m_nodes.find(std::string(node_name)) != this->m_nodes.end()){
        EAGLEEYE_LOGD("Node %s has exist in pipeline", node_name);
        return "";
    }

    GroupNode* gn = new GroupNode();
    // set node name
    gn->setUnitName(node_name);
    // set pipeline
    gn->setPipeline(this);
    // add to 
    this->m_nodes[std::string(node_name)] = gn;
    
    int default_port = 0;
    for(int i=0; i<group_nodes.size(); ++i){
        AnyNode* node_ptr = this->m_nodes[group_nodes[i]];
        gn->setInputPort(node_ptr->getOutputPort(default_port), i);
    }

    return node_name;
}

void AnyPipeline::depend(const char* node, std::vector<std::string> dependent_nodes){
    // 手动添加依赖关系
    // 用于编排无绝对依赖关系的计算节点。（一般用于输出节点的计算顺序）
    std::string node_name = node;
    if(this->m_dependent_nodes.find(node_name) != this->m_dependent_nodes.end()){
        this->m_dependent_nodes[node_name] = std::vector<std::string>();
    }

    for(int i=0; i<dependent_nodes.size(); ++i){
        bool is_finding = false;
        for(int k=0; k<this->m_dependent_nodes[node_name].size(); ++k){
            if(this->m_dependent_nodes[node_name][k] == dependent_nodes[i]){
                is_finding = true;
                break;
            }
        }

        if(!is_finding){
            this->m_dependent_nodes[node_name].push_back(dependent_nodes[i]);
        }
    }
}

AnyNode* AnyPipeline::getNode(std::string node_name){
    if(this->m_nodes.find(node_name) == this->m_nodes.end()){
        return NULL;
    }
    return this->m_nodes[node_name];
}

AnyNode* AnyPipeline::branch(std::string pipeline_name, std::string node_name, int port, bool inplace){
    if(m_pipeline_map.find(pipeline_name) == m_pipeline_map.end()){
        EAGLEEYE_LOGD("not exist pipeline %s", pipeline_name.c_str());
        return NULL;
    }
    if(m_dependent_pipelines.find(pipeline_name) == m_dependent_pipelines.end()){
        EAGLEEYE_LOGD("not approved to use pipeline %s", pipeline_name.c_str());
        return NULL;
    }
    std::vector<std::pair<std::string, std::string>> allowed_nodes = this->m_dependent_pipelines[pipeline_name];
    bool is_ok = false;
    for(int i=0; i<allowed_nodes.size(); ++i){
        if(allowed_nodes[i].first == node_name+"/"+tos(port)){
            is_ok = true;
            break;
        }
    }
    if(is_ok){
        EAGLEEYE_LOGD("not approved to use node %s from pipeline %s", node_name.c_str(), pipeline_name.c_str());
        return NULL;
    }

    std::shared_ptr<AnyPipeline> source_pipeline = m_pipeline_map[pipeline_name];
    AnyNode* source_node = source_pipeline->getNode(node_name);
    if(source_node == NULL){
        EAGLEEYE_LOGD("couldnt get node %s from pipeline %s", node_name.c_str(), pipeline_name.c_str());
        return NULL;
    }

    if(port >= source_node->getNumberOfOutputSignals()){
        EAGLEEYE_LOGD("exceed source node number");
        return NULL;
    }

    CopyNode* copy_node = new CopyNode(inplace);
    copy_node->setInputPort(source_node->getOutputPort(port), 0);
    return copy_node;
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
    // try 1: 严格匹配
    for(iter = this->m_monitor_params.begin(); iter != iend; ++iter){
        std::string separator = "/";
        std::vector<std::string> ab = split(iter->first, separator);
        if(ab[0] == node_name && endswith(iter->first, param_name)){
            iter->second->setVar(value);
            issuccess = true;
        }
    }
    // try 2: 模糊匹配
    if(!issuccess){
        for(iter = this->m_monitor_params.begin(); iter != iend; ++iter){
            if((startswith(iter->first, node_name) && endswith(iter->first, param_name)) || (endswith(iter->first, node_name) && endswith(iter->first, param_name))){
                iter->second->setVar(value);
                issuccess = true;
            }
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

void AnyPipeline::setCallback(const char* node_name, std::function<void(AnyNode*, std::vector<AnySignal*>)> callback){
    if(node_name == NULL || strcmp(node_name, "") == 0){
        EAGLEEYE_LOGE("Node name is empty");
        return;
    }

    AnyNode* node = this->getNode(node_name);
    if(node == NULL){
        EAGLEEYE_LOGE("Node %s not exists.", node_name);
        return;
    }
    node->setCallback(callback);
}

void AnyPipeline::setInput(const char* node_name, 
                           void* data, 
                           const size_t* data_size, 
                           const int data_dims,
                           const int data_rotation,
                           const int data_type){
    if(node_name == NULL || strcmp(node_name, "") == 0){
        EAGLEEYE_LOGE("Node name is empty.");
        return;
    }

    // check m_using_placeholders(用户临时插入的替代性占位符)
    if(this->m_using_placeholders.size() > 0){
        int finding_index = -1;
        for(int i=0; i<this->m_using_placeholders.size(); ++i){
            std::string placeholder_port_name = this->m_using_placeholders[i]->getUnitName();
            placeholder_port_name = placeholder_port_name + "/" +tos(this->m_replace_ports[i]);

            if(placeholder_port_name == std::string(node_name)){
                finding_index = i;
                break;
            }
        }

        if(finding_index != -1){
            EAGLEEYE_LOGD("Set custom placeholder %s.", node_name);
            MetaData meta = m_using_placeholders[finding_index]->getOutputPort(0)->meta();
            meta.rows = data_size[0];
            meta.cols = data_size[1];
            meta.rotation = data_rotation;
            m_using_placeholders[finding_index]->getOutputPort(0)->setData(data, meta);
            m_using_placeholders[finding_index]->modified();
            return;
        }   
    }

    EAGLEEYE_LOGD("Set pipeline input %s.", node_name);
    std::string input_key = std::string(node_name);    
    int port = 0;
    if(input_key.find("/") != std::string::npos){
        std::vector<std::string> kterms = split(input_key, "/");
        input_key = kterms[0];
        port = tof<int>(kterms[1]);
    }

    if(this->m_input_nodes.find(input_key) == this->m_input_nodes.end()){
        EAGLEEYE_LOGE("Node %s is not input node.", node_name);
        return;
    }
    
    for(int d=0; d<data_dims; ++d){
        if(data_size[d] == 0){
            EAGLEEYE_LOGE("Data size abnormal data_size[%d]=0", d);
            return;
        }
    }

    MetaData meta = this->m_input_nodes[input_key]->getOutputPort(port)->meta();
    meta.rows = data_size[0];
    meta.cols = data_size[1];
    meta.rotation = data_rotation;
    this->m_input_nodes[input_key]->getOutputPort(port)->setData(data, meta);
    this->m_input_nodes[input_key]->modified();
    EAGLEEYE_LOGD("Finish set signal content.");
}

void AnyPipeline::setInput(const char* node_name, void* data, MetaData meta){
    if(node_name == NULL || strcmp(node_name, "") == 0){
        EAGLEEYE_LOGE("Node name is empty.");
        return;
    }
    EAGLEEYE_LOGV("Set pipeline input %s.", node_name);
    std::string input_key = std::string(node_name);    
    int port = 0;
    if(input_key.find("/") != std::string::npos){
        std::vector<std::string> kterms = split(input_key, "/");
        input_key = kterms[0];
        port = tof<int>(kterms[1]);
    }

    if(this->m_input_nodes.find(input_key) == this->m_input_nodes.end()){
        EAGLEEYE_LOGE("Node %s is not input node.", node_name);
        return;
    }

    this->m_input_nodes[input_key]->getOutputPort(port)->setData(data, meta);
    this->m_input_nodes[input_key]->modified();
    EAGLEEYE_LOGV("Finish set signal content.");
}

void AnyPipeline::setInput(const char* node_name, std::string from_pipeline_name, std::string from_node_name){
    if(node_name == NULL || strcmp(node_name, "") == 0){
        EAGLEEYE_LOGE("Node name is empty.");
        return;
    }

    EAGLEEYE_LOGD("Set pipeline input %s.", node_name);
    std::string input_key = std::string(node_name);    
    int port = 0;
    if(input_key.find("/") != std::string::npos){
        std::vector<std::string> kterms = split(input_key, "/");
        input_key = kterms[0];
        port = tof<int>(kterms[1]);
    }

    if(this->m_input_nodes.find(input_key) == this->m_input_nodes.end()){
        EAGLEEYE_LOGE("Node %s is not input node.", node_name);
        return;
    }

    std::string sig_name = formatString("%s/%s", from_pipeline_name.c_str(), from_node_name.c_str());
    AnySignal* sig = AnyRegister::get()->signal(sig_name);
    if(sig == NULL){
        return;
    }

    this->m_input_nodes[input_key]->getOutputPort(port)->copy(sig);
    this->m_input_nodes[input_key]->modified();
    EAGLEEYE_LOGD("Finish set input %s", node_name);
}


void AnyPipeline::setInput(const char* node_name, std::string from_register_node){
    if(node_name == NULL || strcmp(node_name, "") == 0){
        EAGLEEYE_LOGE("Node name is empty.");
        return;
    }

    EAGLEEYE_LOGD("Set pipeline input %s.", node_name);
    std::string input_key = std::string(node_name);    
    int port = 0;
    if(input_key.find("/") != std::string::npos){
        std::vector<std::string> kterms = split(input_key, "/");
        input_key = kterms[0];
        port = tof<int>(kterms[1]);
    }

    if(this->m_input_nodes.find(input_key) == this->m_input_nodes.end()){
        EAGLEEYE_LOGE("Node %s is not input node.", node_name);
        return;
    }

    AnySignal* register_sig = AnyRegister::get()->signal(from_register_node);
    if(register_sig == NULL){
        EAGLEEYE_LOGE("Register node not exist.");
        return;
    }
    
    if(this->m_input_nodes[input_key]->getOutputPort(port)->getSignalType() != register_sig->getSignalType()){
        EAGLEEYE_LOGE("Signal type (%s,%s) not consistent.", node_name, from_register_node.c_str());
        return;
    }

    this->m_input_nodes[input_key]->getOutputPort(port)->copy(register_sig);
}

void AnyPipeline::setInputPort(const char* node_name, int node_port, AnySignal* input_sig){
    if(node_name == NULL || strcmp(node_name, "") == 0){
        EAGLEEYE_LOGE("Node name is empty.");
        return;
    }

    std::string input_key = std::string(node_name);    
    int port = node_port;
    if(input_key.find("/") != std::string::npos){
        std::vector<std::string> kterms = split(input_key, "/");
        input_key = kterms[0];
        port = tof<int>(kterms[1]);
    }
    if(this->m_input_nodes.find(input_key) == this->m_input_nodes.end()){
        EAGLEEYE_LOGE("Node %s is not input node.", node_name);
        return;
    }

    this->m_input_nodes[input_key]->setInputPort(input_sig, port);
}

void AnyPipeline::getOutput(const char* node_name, 
                            void*& data, 
                            size_t*& data_size, 
                            int& data_dims,
                            int& data_type){
    if(node_name == NULL || strcmp(node_name, "") == 0){
        EAGLEEYE_LOGE("node name is empty");
        data = NULL;
        data_size = NULL;
        data_dims = 0;
        data_type = -1;
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

void AnyPipeline::getNodeOutput(const char* node_name, void*& data, size_t*& data_size, int& data_dims, int& data_type){
    if(node_name == NULL || strcmp(node_name, "") == 0){
        EAGLEEYE_LOGE("node name is empty");
        data = NULL;
        data_size = NULL;
        data_dims = 0;
        data_type = -1;        
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
        input_types.push_back(iter->second->getOutputPort(0)->getSignalTypeName());
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
                signal_type += std::string(iter->second->getOutputPort(index)->getSignalTypeName()) + "/";
                signal_target += std::string(iter->second->getOutputPort(index)->getSignalTarget()) + "/";
            }
            else{
                signal_type += std::string(iter->second->getOutputPort(index)->getSignalTypeName());
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
        if(iter->second->monitor_category != MONITOR_DEFAULT){
            continue;
        }

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

void AnyPipeline::initialize(const char* resource_folder, std::function<bool()> init_func, bool ignore_version_check){    
    if(!ignore_version_check){
        // 设置基本信息
        if(AnyPipeline::m_pipeline_version.find(this->m_name) == AnyPipeline::m_pipeline_version.end()){
            EAGLEEYE_LOGD("Pipeline %s version not be register.", this->m_name.c_str());
            return;
        }    
        this->m_version = AnyPipeline::m_pipeline_version[this->m_name];
        if(AnyPipeline::m_pipeline_signature.find(this->m_name) == AnyPipeline::m_pipeline_signature.end()){
            EAGLEEYE_LOGD("Pipeline %s signature not be register.", this->m_name.c_str());
            return;
        }
        this->m_signature = AnyPipeline::m_pipeline_signature[this->m_name];
        EAGLEEYE_LOGD("Pipeline %s basic information.", this->m_name.c_str());
        EAGLEEYE_LOGD("Version      %s.", this->m_version.c_str());
        EAGLEEYE_LOGD("Signature    %s.", this->m_signature.c_str());
    }

    // 创建管道资源文件夹
    if(resource_folder != NULL){
        this->setResoruceFolder(resource_folder);
    }

    // 初始化管线
    if(init_func == nullptr && m_init_func != nullptr){
        // 初始化管道结构
        EAGLEEYE_LOGD("Build pipeline %s structure.", this->m_name.c_str());
        m_init_func(NULL);
    }
    else if(init_func != nullptr){
        EAGLEEYE_LOGD("Build pipeline %s structure.", this->m_name.c_str());
        init_func();
    }

    std::string resource_folder_s = this->resourceFolder();
    if(!isdirexist(resource_folder_s.c_str())){
        createdirectory(resource_folder_s.c_str());
    }

    // 分析连接关系
    EAGLEEYE_LOGD("Analyze pipeline %s structure.", this->m_name.c_str());
    std::map<std::string, AnyNode*>::iterator node_iter, node_iend(this->m_nodes.end());
    for(node_iter=this->m_nodes.begin(); node_iter != node_iend; ++node_iter){
        if(node_iter->second->getNumberOfInputSignals() == 0){
            // input node
            this->m_input_nodes[std::string(node_iter->first)] = node_iter->second;
            EAGLEEYE_LOGD("Input node %s.", node_iter->first.c_str());
        }

        bool is_output_node = true;
        for(int sig_i=0; sig_i<node_iter->second->getNumberOfOutputSignals(); ++sig_i){
            if(node_iter->second->getOutputPort(sig_i)->getOutDegree() != 0){
                is_output_node = false;
                break;
            }
        }

        if(is_output_node){
            // output node
            this->m_output_nodes[std::string(node_iter->first)] = node_iter->second;
            EAGLEEYE_LOGD("Output node %s.", node_iter->first.c_str());
        }
    }

    // 分析强制依赖关系
    if(m_dependent_nodes.size() > 0){
        std::map<std::string, std::vector<std::string>>::iterator depend_iter, depend_iend(m_dependent_nodes.end());
        for(depend_iter = m_dependent_nodes.begin(); depend_iter != depend_iend; ++depend_iter){
            for(int k=0; k<depend_iter->second.size(); ++k){
                this->m_nodes[depend_iter->first]->addDependentNode(this->m_nodes[depend_iter->second[k]]);
            }
        }
    }
    this->m_order_nodes.clear();
    std::map<std::string, AnyNode*>::iterator output_iter, output_iend(this->m_output_nodes.end());
    for(output_iter=this->m_output_nodes.begin(); output_iter != output_iend; ++output_iter){
        this->m_order_nodes.push_back(output_iter->first);
    }

    // // 根据手动依赖关系，对输出节点重新排序
    // // m_dependent_nodes,排序后保存在m_order_nodes
    // std::map<std::string, AnyNode*>::iterator output_iter, output_iend(this->m_output_nodes.end());
    // if(m_dependent_nodes.size() > 0){
    //     // 排序
    //     std::vector<std::string> result = eagleeye_topology_sort(m_dependent_nodes);

    //     this->m_order_nodes.clear();
    //     // 记录未构建依赖关系的节点
    //     for(output_iter=this->m_output_nodes.begin(); output_iter != output_iend; ++output_iter){
    //         bool is_finding=false;
    //         for(int i=0; i<result.size(); ++i){
    //             if(output_iter->first == result[i]){
    //                 is_finding = true;
    //                 break;
    //             }
    //         }

    //         if(!is_finding){
    //             this->m_order_nodes.push_back(output_iter->first);
    //         }
    //     }

    //     // 加入排序后的节点
    //     for(int i=0; i<result.size(); ++i){
    //         if(this->m_output_nodes.find(result[i]) != this->m_output_nodes.end()){
    //             this->m_order_nodes.push_back(result[i]);
    //         }
    //     }
    // }
    // else{
    //     this->m_order_nodes.clear();
    //     for(output_iter=this->m_output_nodes.begin(); output_iter != output_iend; ++output_iter){
    //         this->m_order_nodes.push_back(output_iter->first);
    //     }
    // }

    EAGLEEYE_LOGD("Pipeline %s has %d output nodes.", this->m_name.c_str(), this->m_output_nodes.size());
    for(output_iter=this->m_output_nodes.begin(); output_iter != output_iend; ++output_iter){
        EAGLEEYE_LOGD("Output node name %s.", output_iter->first.c_str());
    }
    if(m_output_nodes.size() == 0){
        EAGLEEYE_LOGE("Dont have output node in pipeline.");
        return;
    }

    // 初始化管道所有节点
    EAGLEEYE_LOGD("Initialize all node in pipeline %s.", this->m_name.c_str());
    for(output_iter=this->m_output_nodes.begin(); output_iter != output_iend; ++output_iter){
        output_iter->second->init();
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
            if(monitor_iter->second[i]->monitor_category != MONITOR_DEFAULT){
                continue;
            }

            std::string param_name = monitor_iter->second[i]->monitor_var_text;
            std::string param_key = std::string(node_name) + std::string("/") + std::string(param_name);

            this->m_monitor_params[param_key] = monitor_iter->second[i];
        }
    }

    EAGLEEYE_LOGD("Pipeline %s has %d input nodes.", this->m_name.c_str(), this->m_input_nodes.size());
    std::map<std::string,AnyNode*>::iterator input_iter, input_iend(this->m_input_nodes.end());
    for(input_iter=this->m_input_nodes.begin(); input_iter!=input_iend; ++input_iter){
        EAGLEEYE_LOGD("Input node name %s.", input_iter->first.c_str());
    }


    std::string configure_file_path = resource_folder_s + this->m_name + ".pipeline";
    EAGLEEYE_LOGD("Pipeline %s try to load config from %s.", this->m_name.c_str(), configure_file_path.c_str());
    if(isfileexist(configure_file_path.c_str())){
        this->loadConfigure(configure_file_path);
        EAGLEEYE_LOGD("Finish config file loading.");
    }
}

// void AnyPipeline::addFeadbackRule(const char* trigger_node, int trigger_node_state, const char* response_node, const char* response_action){
//     this->get(response_node)->addFeadbackRule(trigger_node, trigger_node_state, response_action);
// }

void AnyPipeline::setInitFunc(INITIALIZE_PLUGIN_PIPELINE_FUNC func){
    m_init_func = func;
}

void AnyPipeline::replaceAt(std::string node_name, int port){
    if(this->m_nodes.find(node_name) == this->m_nodes.end()){
        EAGLEEYE_LOGD("%s node not in pipeline", node_name.c_str());
        return;
    }
    int output_sig_num = this->m_nodes[node_name]->getNumberOfOutputSignals();
    if(port >= output_sig_num){
        EAGLEEYE_LOGD("%s node only have %d ports but you want to replace at port %d", node_name.c_str(), output_sig_num, port);
        return;
    }

    SignalCategory category = this->m_nodes[node_name]->getOutputPort(port)->getSignalCategory();
    EagleeyeType type = this->m_nodes[node_name]->getOutputPort(port)->getValueType();
    AnyNode* n = this->placeholder(category, type);
    n->setUnitName(this->m_nodes[node_name]->getUnitName());

    std::vector<std::pair<AnyNode*,int>> ll;
    std::map<std::string, AnyNode*>::iterator iter, iend(this->m_output_nodes.end());
    for(iter=this->m_output_nodes.begin(); iter!=iend; ++iter){
        iter->second->findIn(this->m_nodes[node_name]->getOutputPort(port),ll);
    }
    if(ll.size() == 0){
        EAGLEEYE_LOGE("dont finding %s node %d port position", node_name.c_str(), port);
        return;
    }

    for(int i=0; i<ll.size(); ++i){
        AnyNode* influenced_node = ll[i].first;
        int influenced_port = ll[i].second;

        influenced_node->setInputPort(n->getOutputPort(0), influenced_port);
        EAGLEEYE_LOGD("reset node %s port %d link", influenced_node->getUnitName(), influenced_port);
    }

    this->m_replace_nodes.push_back(node_name);
    this->m_replace_ports.push_back(port);
    this->m_using_placeholders.push_back(n);
    EAGLEEYE_LOGD("success replace node %s at port %d", node_name.c_str(), port);
}

void AnyPipeline::restoreAt(std::string node_name, int port){
    if(this->m_nodes.find(node_name) == this->m_nodes.end()){
        EAGLEEYE_LOGD("%s node not in pipeline", node_name.c_str());
        return;
    }
    int output_sig_num = this->m_nodes[node_name]->getNumberOfOutputSignals();
    if(port >= output_sig_num){
        EAGLEEYE_LOGD("%s node only have %d ports but you want to replace at port %d", node_name.c_str(), output_sig_num, port);
        return;
    }

    int replaced_num = this->m_replace_nodes.size();
    bool ok = false;
    int index = -1;
    for(int i=0; i<replaced_num; ++i){
        if(this->m_replace_nodes[i] == node_name && this->m_replace_ports[i] == port){
            ok = true;
            index = i;
            break;
        }
    }

    if(!ok){
        EAGLEEYE_LOGD("%s node %d port not be replaced by placeholder", node_name.c_str(), port);
        return;
    }

    std::vector<std::pair<AnyNode*,int>> ll;
    std::map<std::string, AnyNode*>::iterator iter, iend(this->m_output_nodes.end());
    for(iter=this->m_output_nodes.begin(); iter!=iend; ++iter){
        iter->second->findIn(this->m_using_placeholders[index]->getOutputPort(0),ll);
    }
    if(ll.size() == 0){
        EAGLEEYE_LOGE("dont finding placeholder position");
        return;
    }
    
    for(int i=0; i<ll.size(); ++i){
        AnyNode* influenced_node = ll[i].first;
        int influenced_port = ll[i].second;

        influenced_node->setInputPort(this->m_nodes[node_name]->getOutputPort(port), influenced_port);
        EAGLEEYE_LOGD("reset node %s port %d link", influenced_node->getUnitName(), influenced_port);
    }

    // reset info
    std::vector<std::string> tmp_replace_nodes;
    std::vector<int> tmp_replace_ports;
    std::vector<AnyNode*> tmp_using_placeholders;
    for(int i=0; i<replaced_num; ++i){
        if(i != index){
            tmp_replace_nodes.push_back(m_replace_nodes[i]);
            tmp_replace_ports.push_back(m_replace_ports[i]);
            tmp_using_placeholders.push_back(m_using_placeholders[i]);
        }
    }

    delete m_using_placeholders[index];
    this->m_replace_nodes = tmp_replace_nodes;
    this->m_replace_ports = tmp_replace_ports;
    this->m_using_placeholders = tmp_using_placeholders;
    EAGLEEYE_LOGD("success restore node %s at port %d", node_name.c_str(), port);
}

AnyNode* AnyPipeline::placeholder(SignalCategory category, EagleeyeType type){
    switch (category)
    {
    case SIGNAL_CATEGORY_IMAGE:
        if(type == EAGLEEYE_UCHAR){
            return new Placeholder<ImageSignal<unsigned char>>();
        }
        else if(type == EAGLEEYE_INT){
            return new Placeholder<ImageSignal<int>>();
        }
        else if(type == EAGLEEYE_FLOAT){
            return new Placeholder<ImageSignal<float>>();
        }
        else if(type == EAGLEEYE_RGB){
            return new Placeholder<ImageSignal<Array<unsigned char, 3>>>();
        }
        break;
    case SIGNAL_CATEGORY_STRING:
        return new Placeholder<StringSignal>();
    case SIGNAL_CATEGORY_CONTROL:
        return new Placeholder<BooleanSignal>();
    case SIGNAL_CATEGORY_STATE:
        return new Placeholder<StateSignal>();
    default:
        break;
    }

    return NULL;
}

std::string AnyPipeline::resourceFolder(){
    if(!this->m_resource_folder.empty()){
        return this->m_resource_folder;
    }

    std::string root = "./";
    if(!this->m_plugin_root.empty()){
        if(endswith(this->m_plugin_root, "/") || endswith(this->m_plugin_root, "\\")){
            root = this->m_plugin_root;
        }
        else{
            root = this->m_plugin_root+'/';
        }
    }
    return root + this->m_name + "/resource/";
}

void AnyPipeline::setResoruceFolder(std::string folder){    
    this->m_resource_folder = folder;

    if(!isdirexist(this->m_resource_folder.c_str())){
        createdirectory(this->m_resource_folder.c_str());
    }
}

void AnyPipeline::setPluginRoot(const char* root){
    AnyPipeline::m_plugin_root = root;
}

void AnyPipeline::onRenderSurfaceCreate(){
    if(!AnyPipeline::m_render_context.get()){
        AnyPipeline::m_render_context = 
                std::shared_ptr<RenderContext>(new RenderContext(), 
                                [](RenderContext* ptr){delete ptr;});
    }

#ifdef EAGLEEYE_OPENGL
    // 清空shader环境
    // shader/opengl 环境与工作线程有关
    ShaderManager::getInstance()->clear();
#endif
    // 渲染上下文清空
    AnyPipeline::m_render_context->onCreated();
}

void AnyPipeline::onRenderSurfaceChange(int width, int height, int rotate, bool mirror){
    if(AnyPipeline::m_render_context.get()){
        AnyPipeline::m_render_context->onChanged(width, height, rotate, mirror);
    }
}

void AnyPipeline::onRenderSurfaceMouse(int mouse_x, int mouse_y, int mouse_flag){
    if(AnyPipeline::m_render_context.get()){
        AnyPipeline::m_render_context->onMouse(mouse_x, mouse_y, mouse_flag);
    }
}

int AnyPipeline::getRenderSurfaceW(){
    if(AnyPipeline::m_render_context.get()){
        return AnyPipeline::m_render_context->getScreenW();
    }
    
    return 0;
}

int AnyPipeline::getRenderSurfaceH(){
    if(AnyPipeline::m_render_context.get()){
        return AnyPipeline::m_render_context->getScreenH();
    }
    
    return 0;
}

RenderContext* AnyPipeline::getRenderContext(){
    return AnyPipeline::m_render_context.get();
}
}