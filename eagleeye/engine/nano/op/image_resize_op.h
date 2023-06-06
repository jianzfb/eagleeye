#ifndef _EAGLEEYE_IMAGERESIZE_OP_
#define _EAGLEEYE_IMAGERESIZE_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/interpolate_op.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{

class ResizeOp: public BaseOp<1, 1>,DynamicCreator<ResizeOp>{
public:
    ResizeOp() = default;
    ResizeOp(std::vector<int64_t> out_size, float scale, InterpolateOpType op_type);
    ResizeOp(const ResizeOp& op);
    virtual ~ResizeOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

private:
    std::vector<int64_t> m_out_size;
    float m_scale;
    InterpolateOpType m_op_type;
};

class ResizeWithShapeOp: public BaseOp<2, 1>,DynamicCreator<ResizeWithShapeOp>{
public:
    ResizeWithShapeOp() = default;
    ResizeWithShapeOp(InterpolateOpType op_type);
    ResizeWithShapeOp(const ResizeWithShapeOp& op);
    virtual ~ResizeWithShapeOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

private:
    InterpolateOpType m_op_type;
};
} // namespace dataflow

} // namespace eagleeye

#endif