#include "eagleeye/processnode/SerialReadNode.h"
#include "eagleeye/common/EagleeyeIO.h"
#include "eagleeye/framework/pipeline/AnyMonitor.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/common/EagleeyeLog.h"
namespace eagleeye
{
SerialReadNode::SerialReadNode(EagleeyeType data_type){
    // 输出信号
    this->m_data_type = data_type;
    this->setNumberOfOutputSignals(1);
    switch (this->m_data_type)
    {
    case EAGLEEYE_CHAR:
        this->setOutputPort(new ImageSignal<char>,0);
        break;
    case EAGLEEYE_UCHAR:
        this->setOutputPort(new ImageSignal<unsigned char>,0);
        break;
    case EAGLEEYE_SHORT:
        this->setOutputPort(new ImageSignal<short>,0);
        break;
    case EAGLEEYE_INT:
        this->setOutputPort(new ImageSignal<int>,0);
        break;
    case EAGLEEYE_FLOAT:
        this->setOutputPort(new ImageSignal<float>,0);
        break;
    case EAGLEEYE_RGB:
        this->setOutputPort(new ImageSignal<Array<unsigned char, 3>>,0);
        break;
    default:
        EAGLEEYE_LOGD("dont support type %d", int(data_type));
        m_not_ok = false;
        return;
    }

    m_not_ok = true;
    m_serial_reader = NULL;
    m_folder_or_prefix_update = false;
    EAGLEEYE_MONITOR_VAR(std::string, setFolder, getFolder, "folder", "", "");
    EAGLEEYE_MONITOR_VAR(std::string, setPrefix, getPrefix, "prefix", "", "");
}

SerialReadNode::~SerialReadNode(){
    if(m_serial_reader != NULL){
        delete m_serial_reader;
    }
} 

void SerialReadNode::executeNodeInfo(){
    if(this->m_serial_reader == NULL || m_folder_or_prefix_update){
        if(this->m_serial_reader == NULL){
            this->m_serial_reader = new SerialStringReader(this->m_prefix, this->m_folder);
        }
        else{
            delete m_serial_reader;
            this->m_serial_reader = new SerialStringReader(this->m_prefix, this->m_folder);
        }

        m_folder_or_prefix_update = false;
        this->m_next = this->m_serial_reader->next();
    }
    
    // 1.step read data
    if(this->m_data_type == EAGLEEYE_CHAR){
        Matrix<char> data_char;
        EagleeyeIO::read(data_char, this->m_next, READ_BINARY_MODE);
        ImageSignal<char>* output_char_signal = (ImageSignal<char>*)(this->getOutputPort(0));
        output_char_signal->setData(data_char);
    } 
    else if(this->m_data_type == EAGLEEYE_UCHAR){
        Matrix<unsigned char> data_uchar;
        EagleeyeIO::read(data_uchar, this->m_next, READ_BINARY_MODE);
        ImageSignal<unsigned char>* output_uchar_signal = (ImageSignal<unsigned char>*)(this->getOutputPort(0));
        output_uchar_signal->setData(data_uchar);
    }
    else if(this->m_data_type ==  EAGLEEYE_SHORT){
        Matrix<short> data_short;
        EagleeyeIO::read(data_short, this->m_next, READ_BINARY_MODE);
        ImageSignal<short>* output_short_signal = (ImageSignal<short>*)(this->getOutputPort(0));
        output_short_signal->setData(data_short);
    }
    else if(this->m_data_type ==  EAGLEEYE_INT){
        Matrix<int> data_int;
        EagleeyeIO::read(data_int, this->m_next, READ_BINARY_MODE);
        ImageSignal<int>* output_int_signal = (ImageSignal<int>*)(this->getOutputPort(0));
        output_int_signal->setData(data_int);
    }
    else if(this->m_data_type == EAGLEEYE_FLOAT){
        Matrix<float> data_float;
        EagleeyeIO::read(data_float, this->m_next, READ_BINARY_MODE);
        ImageSignal<float>* output_float_signal = (ImageSignal<float>*)(this->getOutputPort(0));
        output_float_signal->setData(data_float);
    }
    else if(this->m_data_type == EAGLEEYE_RGB){
        Matrix<Array<unsigned char, 3>> data_rgb;
        EagleeyeIO::read(data_rgb, this->m_next, READ_BINARY_MODE);
        ImageSignal<Array<unsigned char, 3>>* output_rgb_signal = (ImageSignal<Array<unsigned char, 3>>*)(this->getOutputPort(0));
        output_rgb_signal->setData(data_rgb);
    }

    // 2.step get next file
    this->m_next = m_serial_reader->next();
}

void SerialReadNode::feadback(std::map<std::string, int>& node_state_map){
    // 1.step call base feadback
    Superclass::feadback(node_state_map);

    // 2.step whether need to update
    if(m_next.size() != 0){
        this->modified();
    }
}

void SerialReadNode::setFolder(std::string folder){
    this->m_folder = folder;
    m_folder_or_prefix_update = true;
    modified();
}

void SerialReadNode::getFolder(std::string& folder){
    folder = this->m_folder;
}

void SerialReadNode::setPrefix(std::string prefix){
    this->m_prefix = prefix;
    m_folder_or_prefix_update = true;
    modified();
}

void SerialReadNode::getPrefix(std::string& prefix){
    prefix = this->m_prefix;
}

bool SerialReadNode::selfcheck(){
    return this->m_not_ok;
}
} // namespace eagleeye
