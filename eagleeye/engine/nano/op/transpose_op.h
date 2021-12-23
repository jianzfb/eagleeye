#ifndef _EAGLEEYE_TRANSPOSE_OP_
#define _EAGLEEYE_TRANSPOSE_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/basic/Dim.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{

class TransposeOp:public BaseOp<Tensor, 1, 1>{
public:
    TransposeOp(){};
    TransposeOp(std::vector<int64_t> axis);
    TransposeOp(const TransposeOp& op);
    
    virtual ~TransposeOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int runOnCpu(std::vector<Tensor> input={});
    virtual int runOnGpu(std::vector<Tensor> input={});

protected:
    void reInitWhenNeeded(Tensor input);

private:
    bool m_need_trans;
    Dim m_last_shape;
    Dim m_new_shape;
    bool m_trans_mat;
    int m_trans_num;
    int m_trans_w;
    int m_trans_h;

    std::vector<int64_t> m_axis;
};

} // namespace dataflow
} // namespace eagleeye


#endif