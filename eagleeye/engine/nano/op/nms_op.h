#ifndef _EAGLEEYE_NMS_OP_
#define _EAGLEEYE_NMS_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
// [(label, confidence, xmin, ymin, xmax, ymax), ()]
class NmsOp:public BaseOp<Tensor, 2, 2>{
public:
    NmsOp(float score_threshold, 
            int nms_top_k, 
            int keep_top_k=-1, 
            float nms_threshold=0.3, 
            float normalized=true, 
            float nms_eta=1.0, 
            int background_label=0,
            bool use_gaussian=false,
            float gaussian_sigma=0.5);
    NmsOp(const NmsOp& op);

    virtual ~NmsOp();

    NmsOp() = default;
    
    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:   
    float m_score_threshold;
    int m_nms_top_k;
    int m_keep_top_k;
    float m_nms_threshold;
    bool m_normalized;
    float m_nms_eta;
    int m_background_label; 
    bool m_use_gaussian;
    float m_gaussian_sigma;
};

} // namespace dataflow
} // namespace eagleeye

#endif