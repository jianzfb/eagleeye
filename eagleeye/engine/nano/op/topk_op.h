#ifndef _EAGLEEYE_TOPK_OP_
#define _EAGLEEYE_TOPK_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class TopKOp: public BaseOp<1, 2>,DynamicCreator<TopKOp>{
public:
    TopKOp():m_k(-1),m_axis(-1){}
    TopKOp(int axis, int k);
    TopKOp(const TopKOp& op);

    virtual ~TopKOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    int m_k;
    int m_axis;
}; 
}    
}
#endif