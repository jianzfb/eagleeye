#ifndef _EAGLEEYE_STREAM_CENTER_H_
#define _EAGLEEYE_STREAM_CENTER_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeMessage.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/processnode/VideoStreamNode.h"
#include "eagleeye/processnode/PlaceholderQueue.h"
#include <map>
#include <functional> 
#include <memory>
#include <mutex>

namespace eagleeye{
class StreamCenter{
public:
    static StreamCenter* getInstance();
    virtual ~StreamCenter();

    AnyNode* createVideoStream(std::string name, int queue_size, std::string encode_mode="H264");
    AnyNode* createStream(std::string name, int queue_size,  std::vector<std::string> input_types, std::vector<std::string> input_categorys);

    AnyNode* getStream(std::string name);
    bool decoce(std::string name, uint8_t* package_data, int package_size);
    bool removeStream(std::string name);

private:
    StreamCenter();
    
    std::mutex m_mu;
    std::map<std::string, AnyNode*> m_stream_info;
    static std::shared_ptr<StreamCenter> m_instance;
};
}


#endif