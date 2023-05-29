#ifndef _EAGLEEYE_WARPAFFINE_OP_
#define _EAGLEEYE_WARPAFFINE_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
enum AffineParamMode{
    RotationMode    = 0,
    AffineMode      = 1,
    TMMode          = 2
};

class WarpAffineOp:public BaseOp<Tensor, 2, 3>{
public:
    WarpAffineOp(AffineParamMode mode, int64_t out_h, int64_t out_w, int fill_type=-233, unsigned int v=0);
    virtual ~WarpAffineOp();

    WarpAffineOp() = default;
    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    int m_fill_type; // 0->区域外填充为v[0],v[1],v[2], -233->区域外不处理
    unsigned int m_v;
    AffineParamMode m_mode;
    int64_t m_out_h;
    int64_t m_out_w;
}; 

} // namespace dataflow

} // namespace eagleeye


#endif