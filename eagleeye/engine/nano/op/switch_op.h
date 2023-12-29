#ifndef _EAGLEEYE_SWITCH_OP_
#define _EAGLEEYE_SWITCH_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/basic/Dim.h"
#include "eagleeye/basic/type.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class SwitchOp:public BaseOp<3, 1>,DynamicCreator<SwitchOp>{
public:
    using BaseOp<3, 1>::init;
    SwitchOp();
    virtual ~SwitchOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

private:
};  
}
}
#endif