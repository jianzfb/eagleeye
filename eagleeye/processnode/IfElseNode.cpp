#include "eagleeye/processnode/IfElseNode.h"

namespace eagleeye{
IfElseNode::IfElseNode(AnyNode* x, AnyNode* y)
    :AnyNode("ifelse"){
    this->setNumberOfInputSignals(1);
    this->m_x = NULL;
    this->m_y = NULL;
    if(x == NULL || y == NULL){
        return;
    }

    // set output port
    int x_output_signal_num = x->getNumberOfOutputSignals();
    this->setNumberOfOutputSignals(x_output_signal_num);
    for(int i=0; i<x_output_signal_num; ++i){
        this->setOutputPort(x->getOutputPort(i)->make(), i);
    }

    std::string x_name =x->getUnitName();
    x->setUnitName((x_name+":TRUE").c_str());
    std::string y_name =y->getUnitName();
    y->setUnitName((y_name+":FALSE").c_str());

    // set input port number
    // 0 - conditon signal
    // 1 - data signal
    // 2 - data signal
    // ...

    this->m_x = x;
    this->m_y = y;
}    

IfElseNode::~IfElseNode(){
    if(this->m_x){
        delete this->m_x;
    }
    if(this->m_y){
        delete this->m_y;
    }
}

void IfElseNode::config(std::function<std::vector<AnyNode*>()> generator){
    // set input port number
    // 0 - conditon signal
    // 1 - data signal
    // 2 - data signal
    // ...
    std::vector<AnyNode*> upper_nodes = generator();
    this->m_x = upper_nodes[0];
    this->m_y = upper_nodes[1];

    int x_output_signal_num = this->m_x->getNumberOfOutputSignals();
    this->setNumberOfOutputSignals(x_output_signal_num);
    for(int i=0; i<x_output_signal_num; ++i){
        this->setOutputPort(this->m_x->getOutputPort(i)->make(), i);
    }

    std::string x_name =this->m_x->getUnitName();
    this->m_x->setUnitName((x_name+":TRUE").c_str());
    std::string y_name =this->m_y->getUnitName();
    this->m_y->setUnitName((y_name+":FALSE").c_str());
}

void IfElseNode::executeNodeInfo(){
    // get input / output signal
    // 1.step 条件信号
    BooleanSignal* condition_sig = (BooleanSignal*)(this->getInputPort(0));
    if(condition_sig->getData()){
        // 执行X
        // 赋值所有输入信号到X
        EAGLEEYE_LOGD("true branch in IFELSE");
        this->m_x->start();

        int output_sig_num = this->getNumberOfOutputSignals();
        for(int i=0; i<output_sig_num; ++i){
            this->getOutputPort(i)->copy(this->m_x->getOutputPort(i));
        }
    }
    else{
        // 执行Y
        EAGLEEYE_LOGD("false branch in IFELSE");
        this->m_y->start();
        int output_sig_num = this->getNumberOfOutputSignals();
        for(int i=0; i<output_sig_num; ++i){
            this->getOutputPort(i)->copy(this->m_y->getOutputPort(i));
        }
    }
}

bool IfElseNode::selfcheck(){
    return true;
}

void IfElseNode::init(){
    Superclass::init();

    // 分别x,y初始化    
    this->m_x->init();
    this->m_y->init();
}

void IfElseNode::reset(){
    this->m_x->reset();
    this->m_y->reset();

    Superclass::reset();
}

void IfElseNode::exit(){
    this->m_x->exit();
    this->m_y->exit();
    Superclass::exit();
}


void IfElseNode::getPipelineMonitors(std::map<std::string,std::vector<AnyMonitor*>>& pipeline_monitor_pool){
    // collect all node monitors in subpipeline
    std::map<std::string,std::vector<AnyMonitor*>> temp;
    this->m_x->getPipelineMonitors(temp);
    // 给temp添加更新时的前置回调函数
    std::map<std::string,std::vector<AnyMonitor*>>::iterator monitor_iter, monitor_iend(temp.end());
    for(monitor_iter = temp.begin(); monitor_iter != monitor_iend; ++monitor_iter){
        for(int i=0; i<monitor_iter->second.size(); ++i){
            monitor_iter->second[i]->setPrefixCallback(
                [this](){
                    this->modified();
                }
            );
        }
    }
    pipeline_monitor_pool.insert(temp.begin(), temp.end());
    temp.clear();

    this->m_y->getPipelineMonitors(temp);
    // 给temp添加更新时的前置回调函数
    monitor_iend = temp.end();
    for(monitor_iter = temp.begin(); monitor_iter != monitor_iend; ++monitor_iter){
        for(int i=0; i<monitor_iter->second.size(); ++i){
            monitor_iter->second[i]->setPrefixCallback(
                [this](){
                    this->modified();
                }
            );
        }
    }
    pipeline_monitor_pool.insert(temp.begin(), temp.end());
    temp.clear();   

	//traverse the whole pipeline
	std::vector<AnySignal*>::iterator signal_iter,signal_iend(m_input_signals.end());
	for (signal_iter = m_input_signals.begin();signal_iter != signal_iend; ++signal_iter){
		if ((*signal_iter))
			(*signal_iter)->getPipelineMonitors(pipeline_monitor_pool);
	}
}

void IfElseNode::loadConfigure(std::map<std::string, std::shared_ptr<char>> nodes_config){
    this->m_x->loadConfigure(nodes_config);
    this->m_y->loadConfigure(nodes_config);
    Superclass::loadConfigure(nodes_config);
}

void IfElseNode::saveConfigure(std::map<std::string, std::shared_ptr<char>>& nodes_config){
    this->m_x->saveConfigure(nodes_config);
    this->m_y->saveConfigure(nodes_config);
    Superclass::saveConfigure(nodes_config);
}
}