#ifndef _EAGLEEYE_SPLIT_OP_H_
#define _EAGLEEYE_SPLIT_OP_H_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class Split2DOp:public BaseOp<1, 2>,DynamicCreator<Split2DOp>{
public:
    Split2DOp(int axis);
    Split2DOp(const Split2DOp& op);

    virtual ~Split2DOp();

    Split2DOp() = default;
    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

    int m_axis;
    int m_num;
};

} // namespace dataflow

} // namespace eagleeye

#endif