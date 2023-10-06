#ifndef _EAGLEEYE_PREPROCESS_OP_H_
#define _EAGLEEYE_PREPROCESS_OP_H_

#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class PreprocessOp:public BaseOp<1, 1>,DynamicCreator<PreprocessOp>{
public:
    using BaseOp<1, 1>::init;
    PreprocessOp(std::vector<float> mean_v, std::vector<float> scale_v, bool reverse_channel);
    PreprocessOp(const PreprocessOp& op);
    virtual ~PreprocessOp();

    PreprocessOp() = default;
    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    std::vector<float> m_mean_v;
    std::vector<float> m_scale_v;

    bool m_reverse_channel;
};
}
}
#endif
