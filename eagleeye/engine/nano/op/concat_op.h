#ifndef _EAGLEEYE_CONCAT_OP_H_
#define _EAGLEEYE_CONCAT_OP_H_

#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class ConcatOp:public BaseOp<2, 1>,DynamicCreator<ConcatOp>{
public:
    using BaseOp<2, 1>::init;
    ConcatOp(int aixs);
    ConcatOp(const ConcatOp& op);
    virtual ~ConcatOp();

    ConcatOp() = default;

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    int m_aixs;
};
}
}

#endif