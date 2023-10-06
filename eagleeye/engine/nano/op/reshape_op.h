#ifndef _EAGLEEYE_RESHPAE_OP_
#define _EAGLEEYE_RESHPAE_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{

class ReshapeOp:public BaseOp<1, 1>,DynamicCreator<ReshapeOp>{
public:
    using BaseOp<1, 1>::init;
    ReshapeOp(){}
    ReshapeOp(std::vector<int64_t> shape, bool in_place);
    ReshapeOp(const ReshapeOp& op);
    virtual ~ReshapeOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

private:
    std::vector<int64_t> m_shape;   // 目标形状  
    bool m_in_place;
};

} // namespace dataflow
} // namespace eagleeye


#endif