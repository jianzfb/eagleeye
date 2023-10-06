#ifndef _EAGLEEYE_YUV_OP_
#define _EAGLEEYE_YUV_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class YuvOp: public BaseOp<2, 1>,DynamicCreator<YuvOp>{
public:
    using BaseOp<2, 1>::init;
    YuvOp();
    YuvOp(const YuvOp& op);

    virtual ~YuvOp(){}

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    int m_mode;
};
}    
}
#endif