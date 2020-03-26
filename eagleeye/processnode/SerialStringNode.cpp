#include "eagleeye/processnode/SerialStringNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/common/EagleeyeSerial.h"

namespace eagleeye
{
SerialStringNode::SerialStringNode(){
    // 输出信号
    this->setNumberOfOutputSignals(1);
	this->setOutputPort(new StringSignal(),0);
    EAGLEEYE_MONITOR_VAR(std::string, setFolder, getFolder, "folder","","");
    EAGLEEYE_MONITOR_VAR(std::string, setPrefix, getPrefix, "prefix","","");

    m_serial_string_reader = NULL;
    m_is_change = false;
}   

SerialStringNode::~SerialStringNode(){
    if(m_serial_string_reader != NULL){
        delete m_serial_string_reader;
    }
}

void SerialStringNode::executeNodeInfo(){
    StringSignal* string_sig = (StringSignal*)this->getOutputPort(0);
    if(m_serial_string_reader != NULL && m_is_change){
        delete m_serial_string_reader;
        m_serial_string_reader = NULL;
    }
    
    if(m_serial_string_reader == NULL){
        m_serial_string_reader = new SerialStringReader(this->m_prefix, this->m_folder);
        m_is_change = false;
    }

    if(m_serial_string_reader->count() == 0){
        EAGLEEYE_LOGD("couldnt find any string");
        return;
    }

    if(m_serial_string_reader->cur() < m_serial_string_reader->count()){
        string_sig->setData(m_serial_string_reader->next());
    }
}

void SerialStringNode::setFolder(std::string folder_path){
    this->m_folder = folder_path;
    this->m_is_change = true;
    modified();
}

void SerialStringNode::getFolder(std::string& folder_path){
    folder_path = this->m_folder;
}

void SerialStringNode::setPrefix(std::string prefix){
    this->m_prefix = prefix;
    this->m_is_change = true;
    modified();
}
void SerialStringNode::getPrefix(std::string& prefix){
    prefix = this->m_prefix;    
}

void SerialStringNode::feadback(std::map<std::string, int>& node_state_map){
    // 1.step call base feadback
    Superclass::feadback(node_state_map);

    // 2.step whether need to update
    if(m_serial_string_reader->cur() < this->m_serial_string_reader->count()){
        this->modified();
    }
}
} // namespace eagleeye
