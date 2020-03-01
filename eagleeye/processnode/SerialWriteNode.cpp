#include "eagleeye/processnode/SerialWriteNode.h"
#include "eagleeye/common/EagleeyeIO.h"
#include "eagleeye/framework/pipeline/AnyMonitor.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeFile.h"
namespace eagleeye
{
SerialWriteNode::SerialWriteNode(EagleeyeType data_type){
    // 输出信号
    this->m_data_type = data_type;
    this->setNumberOfInputSignals(1);
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new ImageSignal<unsigned char>, 0);

    this->m_order_index = 0;
    m_folder_or_prefix_update = false;
    EAGLEEYE_MONITOR_VAR(std::string, setFolder, getFolder, "folder", "", "");
    EAGLEEYE_MONITOR_VAR(std::string, setPrefix, getPrefix, "prefix", "", "");
}

SerialWriteNode::~SerialWriteNode(){
} 

void SerialWriteNode::executeNodeInfo(){
    if(this->m_folder_or_prefix_update){
        if(!isdirexist(this->m_folder.c_str())){ 
            createdirectory(this->m_folder.c_str());
        }
        this->m_folder_or_prefix_update = false;
        this->m_order_index = 0;
    }

    char suffix[100];
	sprintf(suffix, "_%d", this->m_order_index);
    std::string output_name = this->m_folder + "/" + this->m_prefix + suffix;

    // 1.step read data
    if(this->m_data_type == EAGLEEYE_CHAR){
        ImageSignal<char>* input_char_signal = (ImageSignal<char>*)(this->getInputPort(0));
        Matrix<char> data_char = input_char_signal->getData();
        EagleeyeIO::write(data_char, output_name.c_str(), WRITE_BINARY_MODE);
    } 
    else if(this->m_data_type == EAGLEEYE_UCHAR){
        ImageSignal<unsigned char>* input_uchar_signal = (ImageSignal<unsigned char>*)(this->getInputPort(0));
        Matrix<unsigned char> data_uchar = input_uchar_signal->getData();
        EagleeyeIO::write(data_uchar, output_name.c_str(), WRITE_BINARY_MODE);
    }
    else if(this->m_data_type ==  EAGLEEYE_SHORT){
        ImageSignal<unsigned char>* input_uchar_signal = (ImageSignal<unsigned char>*)(this->getInputPort(0));
        Matrix<unsigned char> data_uchar = input_uchar_signal->getData();
        EagleeyeIO::write(data_uchar, output_name.c_str(), WRITE_BINARY_MODE);
    }
    else if(this->m_data_type ==  EAGLEEYE_INT){
        ImageSignal<int>* input_int_signal = (ImageSignal<int>*)(this->getInputPort(0));
        Matrix<int> data_int = input_int_signal->getData();
        EagleeyeIO::write(data_int, output_name.c_str(), WRITE_BINARY_MODE);
    }
    else if(this->m_data_type == EAGLEEYE_FLOAT){
        ImageSignal<float>* input_float_signal = (ImageSignal<float>*)(this->getInputPort(0));
        Matrix<float> data_float = input_float_signal->getData();
        EagleeyeIO::write(data_float, output_name.c_str(), WRITE_BINARY_MODE);
    }
    else if(this->m_data_type == EAGLEEYE_RGB){
        ImageSignal<Array<unsigned char,3>>* input_rgb_signal = (ImageSignal<Array<unsigned char,3>>*)(this->getInputPort(0));
        Matrix<Array<unsigned char,3>> data_rgb = input_rgb_signal->getData();
        EagleeyeIO::write(data_rgb, output_name.c_str(), WRITE_BINARY_MODE);
    }

    this->m_order_index += 1;
}

void SerialWriteNode::setFolder(std::string folder){
    this->m_folder = folder;
    m_folder_or_prefix_update = true;
    modified();
}

void SerialWriteNode::getFolder(std::string& folder){
    folder = this->m_folder;
}

void SerialWriteNode::setPrefix(std::string prefix){
    this->m_prefix = prefix;
    m_folder_or_prefix_update = true;
    modified();
}

void SerialWriteNode::getPrefix(std::string& prefix){
    prefix = this->m_prefix;
}
} // namespace eagleeye
