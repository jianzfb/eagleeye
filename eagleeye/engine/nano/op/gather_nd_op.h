#ifndef _EAGLEEYE_GATHER_ND_OP_
#define _EAGLEEYE_GATHER_ND_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class GatherNdOp:public BaseOp<2, 1>,DynamicCreator<GatherNdOp>{
public:
    GatherNdOp(const GatherNdOp& op);

    virtual ~GatherNdOp(){};

    GatherNdOp() = default;

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:    
};
}    
}

#endif