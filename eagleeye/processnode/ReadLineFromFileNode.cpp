#include "eagleeye/processnode/ReadLineFromFileNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeFile.h"
#include "eagleeye/common/EagleeyeStr.h"
#include <fstream>

namespace eagleeye
{
ReadLineFromFileNode::ReadLineFromFileNode(){
    // 输出信号
    this->setNumberOfOutputSignals(1);
	this->setOutputPort(new StringSignal(),0);
    EAGLEEYE_MONITOR_VAR(std::string, setFilePath, getFilePath, "file","","");
}   

ReadLineFromFileNode::~ReadLineFromFileNode(){

}

void ReadLineFromFileNode::executeNodeInfo(){
    StringSignal* string_sig = (StringSignal*)this->getOutputPort(0);

    if(m_file_list.size() == 0){
        std::ifstream file_in(this->m_file_path);
	    std::string line;
        while (std::getline(file_in, line)){
            if(!line.empty()){
                m_file_list.push_back(line);   
            }
        }

        m_cur_index = 0;
    }

    if(m_cur_index < m_file_list.size()){
        string_sig->setData(m_file_list[m_cur_index]);        
    } 

    m_cur_index += 1;
}

void ReadLineFromFileNode::setFilePath(std::string file_path){
    this->m_file_path = file_path;
    this->m_file_list.clear();
    modified();
}

void ReadLineFromFileNode::getFilePath(std::string& file_path){
    file_path = this->m_file_path;
}

// void ReadLineFromFileNode::feadback(std::map<std::string, int>& node_state_map){
//     // 1.step call base feadback
//     Superclass::feadback(node_state_map);

//     // 2.step whether need to update
//     if(m_cur_index < this->m_file_list.size()){
//         this->modified();
//     }
// }

bool ReadLineFromFileNode::finish(){
    // 1.step call base feadback
    Superclass::finish();

    if(m_cur_index < this->m_file_list.size()){
        this->modified();
        return false;
    }
    return true;
}

} // namespace eagleeye
