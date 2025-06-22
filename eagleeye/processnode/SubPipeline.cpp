#include "eagleeye/processnode/SubPipeline.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/AnyPipeline.h"
#include "eagleeye/common/EagleeyeLog.h"
namespace eagleeye
{
SubPipeline::SubPipeline(bool copy_input){
    m_copy_input = copy_input;
}

SubPipeline::~SubPipeline(){
    std::map<std::string, AnyNode*>::iterator iter, iend(this->m_subpipeline.end());
    for(iter = this->m_subpipeline.begin(); iter != iend; ++iter){
        delete iter->second;
    }

    std::vector<AnySignal*>::iterator sig_iter, sig_iend(this->m_cache.end());
    for(sig_iter = this->m_cache.begin(); sig_iter != sig_iend; ++sig_iter){
        delete *sig_iter;
    }
}

void SubPipeline::executeNodeInCopyInputMode(){
    // 1.step copy input
    if(this->m_cache.size() == 0){
        for(int i=0; i<m_input_node_name_list.size(); ++i){
            int input_port_i = m_input_node_port_list[i].first;
            int inner_node_port = m_input_node_port_list[i].second;

            AnySignal* signal_cp = this->getInputPort(input_port_i)->make();
            this->m_cache.push_back(signal_cp);

            std::string inner_node_name =  m_input_node_name_list[i];
            // 动态设置
            if(this->m_subpipeline[inner_node_name]->getNumberOfInputSignals() < inner_node_port+1){
                this->m_subpipeline[inner_node_name]->setNumberOfInputSignals(inner_node_port+1);
            }
            this->m_subpipeline[inner_node_name]->setInputPort(signal_cp, inner_node_port);
        }
    }

    for(int i=0; i<m_input_node_name_list.size(); ++i){
        int input_port_i = m_input_node_port_list[i].first;
        int inner_node_port = m_input_node_port_list[i].second;
        std::string inner_node_name =  m_input_node_name_list[i];

        this->m_subpipeline[inner_node_name]->getInputPort(inner_node_port)->copy(this->getInputPort(input_port_i));
    }

    // 2.step run subpipeline
    std::map<std::string, bool> run_status;
    for(int i=0; i<m_output_node_name_list.size(); ++i){
        std::string name = m_output_node_name_list[i];
        if(run_status.find(name) == run_status.end()){
            run_status[name] = false;
        }
        if(!run_status[name]){
            m_subpipeline[name]->start();
            run_status[name] = true;
        }
    }

    // 3.step copy output
    for(int i=0; i<m_output_node_name_list.size(); ++i){
        std::string inner_node_name = m_output_node_name_list[i];
        int inner_node_port = m_output_node_port_list[i].first;
        int output_port_i = m_output_node_port_list[i].second;
        this->getOutputPort(output_port_i)->copy(m_subpipeline[inner_node_name]->getOutputPort(inner_node_port));
    }
}

void SubPipeline::executeNodeInNoCopyInputMode(){
    // TODO，需要根据50x8运动项目，进行调试为什么存在此模式
    // 1.step copy input
    int input_port_i = 0;
    for(int i=0; i<m_input_node_name_list.size(); ++i){
        std::string name = m_input_node_name_list[i];
        for(int j=0; j<m_subpipeline[name]->getNumberOfInputSignals(); ++j){
            this->m_subpipeline[name]->setInputPort(getInputPort(input_port_i), j);
            input_port_i += 1;
        }
    }

    // 2.step run subpipeline
    for(int i=0; i<m_output_node_name_list.size(); ++i){
        std::string name = m_output_node_name_list[i];
        m_subpipeline[name]->start();
    }

    // 3.step copy output
    int output_port_i = 0;
    for(int i=0; i<m_output_node_name_list.size(); ++i){
        std::string name = m_output_node_name_list[i];
        for(int j=0; j<m_subpipeline[name]->getNumberOfOutputSignals(); ++j){
            this->getOutputPort(output_port_i)->copy(m_subpipeline[name]->getOutputPort(j));
            this->getOutputPort(output_port_i)->setSignalType(m_subpipeline[name]->getOutputPort(j)->getSignalType());
            output_port_i += 1;
        }
    }
}

void SubPipeline::executeNodeInfo(){
    // 动态根据输入设置
    if(m_copy_input){
        executeNodeInCopyInputMode();
    }
    else{
        executeNodeInNoCopyInputMode();
    }
}

void SubPipeline::setUnitName(const char* unit_name){ 
    this->m_unit_name=std::string("subpipeline-") + unit_name;
}

void SubPipeline::add(AnyNode* node, std::string name){
    if(this->m_subpipeline.find(name) == this->m_subpipeline.end()){
        m_subpipeline[name] = node;
        node->setUnitName(name.c_str());
        m_name_list.push_back(name);
    }

    m_node_bind_input_num[name] = 0;
    m_node_bind_output_num[name] = 0;
}

void SubPipeline::bind(std::string fromname, int fromport, std::string toname, int toport){
    // 保留 head, tail 关键字
    if(toname != "tail"){
        if(m_subpipeline.find(toname) == m_subpipeline.end()){
            EAGLEEYE_LOGE("Node %s not int subpipeline.", toname.c_str());
            return;
        }
    }
    if(fromname != "head"){
        if(m_subpipeline.find(fromname) == m_subpipeline.end()){
            EAGLEEYE_LOGE("Node %s not int subpipeline.", fromname.c_str());
            return;
        }
    }
    if(fromname == "head"){
        m_input_node_name_list.push_back(toname);
        m_input_node_port_list.push_back({fromport, toport});

        // 配置输入端口信息（个数）
        if(this->getNumberOfInputSignals() < fromport+1){
            this->setNumberOfInputSignals(fromport+1);
        }
        return;
    }
    if(toname == "tail"){
        m_output_node_name_list.push_back(fromname);
        m_output_node_port_list.push_back({fromport, toport});

        if(this->getNumberOfOutputSignals() < toport+1){
            this->setNumberOfOutputSignals(toport+1);
        }
        // 配置输出端口信号
        this->setOutputPort(this->m_subpipeline[fromname]->getOutputPort(fromport)->make(), toport);
        return;
    }

    AnyNode* node_to_ptr = this->m_subpipeline[std::string(toname)];
    AnyNode* node_from_ptr = this->m_subpipeline[std::string(fromname)];
    if(node_to_ptr->getNumberOfInputSignals() < toport + 1){
        node_to_ptr->setNumberOfInputSignals(toport + 1);
    }
    node_to_ptr->setInputPort(node_from_ptr->getOutputPort(fromport), toport);

    m_node_bind_output_num[fromname] += 1;
    m_node_bind_input_num[toname] += 1;
}

void SubPipeline::analyze(){
    if(m_input_node_name_list.size() != 0 || m_output_node_name_list.size() != 0){
        EAGLEEYE_LOGD("Has use 'head/tail' keyword auto construct, skip analyze here!");
        return;
    }

    // TODO，目前仅支持顺序关联subpipeline -> inner node -> subpipeline
    // inner node 需要设置input signal number(大部分node对于输入端口是动态的)
    int total_input_signal_num = 0;
    int total_output_signal_num = 0;
    m_input_node_name_list.clear();
    m_output_node_name_list.clear();
    for(int i=0; i<m_name_list.size(); ++i){
        std::string node_name = m_name_list[i];
        if(m_node_bind_input_num[node_name] == 0){
            // 获得输入节点相关信息
            int input_signal_num = this->m_subpipeline[node_name]->getNumberOfInputSignals();
            if(input_signal_num == 0){
                EAGLEEYE_LOGE("why > subpipeline %s node 0 input", node_name.c_str());
            }
            for(int j=total_input_signal_num; j<total_input_signal_num+input_signal_num; ++j){
                m_input_node_port_list.push_back({j, j-total_input_signal_num});
                m_input_node_name_list.push_back(node_name);                
            }
            total_input_signal_num += input_signal_num;

            // 设置subpipeline输入端口数
            this->setNumberOfInputSignals(total_input_signal_num);
        }

        if(m_node_bind_output_num[node_name] == 0){
            // 获得输出节点相关信息
            int before_output_signal_num = total_output_signal_num;
            int ouput_signal_num = this->m_subpipeline[node_name]->getNumberOfOutputSignals();
            if(ouput_signal_num == 0){
                EAGLEEYE_LOGE("why > subpipeline %s node 0 output", node_name.c_str());
            }
            for(int j=before_output_signal_num; j<before_output_signal_num+ouput_signal_num; ++j){
                m_output_node_port_list.push_back({j-before_output_signal_num, j});
                m_output_node_name_list.push_back(node_name);
            }
            total_output_signal_num += ouput_signal_num;

            // 设置subpipeline输出端口数
            this->setNumberOfOutputSignals(total_output_signal_num);

            // 配置输出端口信号
            for(int j=before_output_signal_num; j<before_output_signal_num+ouput_signal_num; ++j){
                this->setOutputPort(this->m_subpipeline[node_name]->getOutputPort(j-before_output_signal_num)->make(), j);
            }
        }
    }
}

void SubPipeline::reset(){
    for(int i=0; i<m_output_node_name_list.size(); ++i){
        std::string name = m_output_node_name_list[i];
        m_subpipeline[name]->reset();
    }
    Superclass::reset();
}

void SubPipeline::init(){
    Superclass::init();

    // 初始化子管线
    for(int i=0; i<m_output_node_name_list.size(); ++i){
        std::string name = m_output_node_name_list[i];
        m_subpipeline[name]->init();
    }
}

void SubPipeline::exit(){
    for(int i=0; i<m_output_node_name_list.size(); ++i){
        std::string name = m_output_node_name_list[i];
        m_subpipeline[name]->exit();
    }
    Superclass::exit();
}

void SubPipeline::getPipelineMonitors(std::map<std::string,std::vector<AnyMonitor*>>& pipeline_monitor_pool){
    for(int i=0; i<m_output_node_name_list.size(); ++i){
        std::string name = m_output_node_name_list[i];
        m_subpipeline[name]->getPipelineMonitors(pipeline_monitor_pool);
    }

	//traverse the whole pipeline
	std::vector<AnySignal*>::iterator signal_iter,signal_iend(m_input_signals.end());
	for (signal_iter = m_input_signals.begin();signal_iter != signal_iend; ++signal_iter){
		if ((*signal_iter))
			(*signal_iter)->getPipelineMonitors(pipeline_monitor_pool);
	}
}


void SubPipeline::setCallback(std::string name, std::function<void(AnyNode*, std::vector<AnySignal*>)> callback){
    AnyNode* node = NULL;
    std::string node_name = "";
    std::string next_name = "";
    if(name.find("/") == std::string::npos){
        if(m_subpipeline.find(name) != m_subpipeline.end()){
            node = m_subpipeline[name];

            node_name = name;
            next_name = "";
        }
    }
    else{
        std::string separator = "/";
        std::vector<std::string> name_tree = split(name, separator);
        if(m_subpipeline.find(name) != m_subpipeline.end()){
            node = m_subpipeline[name_tree[0]];

            node_name = name_tree[0];
            next_name = "";
            for(int i=1; i<name_tree.size(); ++i){
                if(i != name_tree.size() - 1){
                    next_name += name_tree[i]+"/";
                }
                else{
                    next_name += name_tree[i];
                }
            }
        }
    }

    if(node == NULL){
        EAGLEEYE_LOGD("Node %s not exists, couldnt set callback.", node_name.c_str());
        return;
    }
    node->setCallback(next_name, callback);
}

} // namespace eagleeye
