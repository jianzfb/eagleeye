#ifndef _EAGLEEYE_PREPROCESS_OP_H_
#define _EAGLEEYE_PREPROCESS_OP_H_

#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class PreprocessOp:public BaseOp<Tensor, 1, 1>{
public:
    PreprocessOp(){}
    PreprocessOp(std::vector<float> mean_v, std::vector<float> scale_v, bool reverse_channel);
    PreprocessOp(const PreprocessOp& op);
    virtual ~PreprocessOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int runOnCpu(std::vector<Tensor> input={});
    virtual int runOnGpu(std::vector<Tensor> input={});

protected:
    void bgrToTensorCHW(const uint8_t* src,
                       float* output,
                       int width,
                       int height,
                       float* means,
                       float* scales);
    void bgrToRgbTensorCHW(const uint8_t* src,
                       float* output,
                       int width,
                       int height,
                       float* means,
                       float* scales);

    std::vector<float> m_mean_v;
    std::vector<float> m_scale_v;

    bool m_reverse_channel;
};
}
}
#endif
