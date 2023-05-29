#ifndef _EAGLEEYE_CAST_OP_H_
#define _EAGLEEYE_CAST_OP_H_

#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class CastOp:public BaseOp<Tensor, 1, 1>{
public:
    CastOp(EagleeyeType data_type, float scale=1.0f);
    CastOp(const CastOp& op);
    virtual ~CastOp();

    CastOp() = default;

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    EagleeyeType m_data_type;
    float m_scale;
};
}
}

#endif