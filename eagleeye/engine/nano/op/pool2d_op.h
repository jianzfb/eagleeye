#ifndef _EAGLEEYE_MAXPOOL2D_OP_
#define _EAGLEEYE_MAXPOOL2D_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
enum Pool2dType{
    MAXPool2D = 0,
    AVGPool2D
};

class Pool2dOp:public BaseOp<Tensor, 1,1>{
public:
    Pool2dOp(Pool2dType pool_type, int ksize_h, int ksize_w, int stride_h, int stride_w, int padding_h, int padding_w);
    Pool2dOp(const Pool2dOp& op);

    virtual ~Pool2dOp();

    Pool2dOp() = default;
    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    int m_ksize_h;
    int m_ksize_w;
    int m_stride_h;
    int m_stride_w;
    int m_padding_h;
    int m_padding_w;
    Pool2dType m_pool_type;
};
}    
}
#endif