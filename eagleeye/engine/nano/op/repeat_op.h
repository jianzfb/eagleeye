#ifndef _EAGLEEYE_REPEAT_OP_
#define _EAGLEEYE_REPEAT_OP_

#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class RepeatOp:public BaseOp<1, 1>,DynamicCreator<RepeatOp>{
public:
    RepeatOp(){};
    RepeatOp(int repeat_times, int axis);
    RepeatOp(const RepeatOp& op);
    virtual ~RepeatOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    int m_axis;
    int m_repeat_times;
};
}
}

#endif