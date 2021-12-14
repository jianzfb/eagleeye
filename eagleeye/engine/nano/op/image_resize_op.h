#ifndef _EAGLEEYE_IMAGERESIZE_OP_
#define _EAGLEEYE_IMAGERESIZE_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/interpolate_op.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{

class ImageResizeOp: public BaseOp<Tensor, 1, 1>{
public:
    ImageResizeOp(std::vector<int64_t> out_size, float scale, InterpolateOpType op_type);
    ImageResizeOp(const ImageResizeOp& op);
    virtual ~ImageResizeOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int runOnCpu(std::vector<Tensor> input={});
    virtual int runOnGpu(std::vector<Tensor> input={});

private:
    std::vector<int64_t> m_out_size;
    float m_scale;
    InterpolateOpType m_op_type;
};

class ImageResizeWithShapeOp: public BaseOp<Tensor, 2, 1>{
public:
    ImageResizeWithShapeOp(InterpolateOpType op_type);
    ImageResizeWithShapeOp(const ImageResizeWithShapeOp& op);
    virtual ~ImageResizeWithShapeOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int runOnCpu(std::vector<Tensor> input={});
    virtual int runOnGpu(std::vector<Tensor> input={});

private:
    InterpolateOpType m_op_type;
};
} // namespace dataflow

} // namespace eagleeye

#endif