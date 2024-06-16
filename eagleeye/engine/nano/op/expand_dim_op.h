#ifndef _EAGLEEYE_EXPAND_DIM_OP_H_
#define _EAGLEEYE_EXPAND_DIM_OP_H_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class ExpandDimOp:public BaseOp<1,1>, DynamicCreator<ExpandDimOp>{
public:
    using BaseOp<1,1>::init;
    ExpandDimOp();
    virtual ~ExpandDimOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params);

    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

private:
    int m_axis;

};
}    
}
#endif