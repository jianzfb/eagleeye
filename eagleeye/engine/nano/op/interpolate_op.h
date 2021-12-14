#ifndef _EAGLEEYE_INTERPOLATE_OP_
#define _EAGLEEYE_INTERPOLATE_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
enum InterpolateOpType{
    INTERPOLATE_BILINER = 0,
    INTERPOLATE_NEAREST = 1
};

class InterpolateOp:public BaseOp<Tensor, 1, 1>{
public:
    InterpolateOp(){}
    InterpolateOp(std::vector<int64_t> out_size, float scale, bool align_corner, InterpolateOpType op_type);
    InterpolateOp(const InterpolateOp& op);
    virtual ~InterpolateOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int runOnCpu(std::vector<Tensor> input={});
    virtual int runOnGpu(std::vector<Tensor> input={});

private:
    std::vector<int64_t> m_out_size;
    float m_scale;
    InterpolateOpType m_op_type;
    bool m_with_align;
    int m_align_mode;
};

class BilinearInterpolateOp:public InterpolateOp{
public:
    BilinearInterpolateOp(std::vector<int64_t> out_size={}, float scale=0.0f, bool align_corner=true)
        :InterpolateOp(out_size, scale, align_corner, INTERPOLATE_BILINER){

        };
    virtual ~BilinearInterpolateOp(){};
};

class NearestInterpolateOp:public InterpolateOp{
public:
    NearestInterpolateOp(std::vector<int64_t> out_size={}, float scale=0.0f, bool align_corner=true)
        :InterpolateOp(out_size, scale, align_corner, INTERPOLATE_NEAREST){

        };
    virtual ~NearestInterpolateOp(){};
};

class InterpolateWithShapeOp: public BaseOp<Tensor, 2, 1>{
public:
    InterpolateWithShapeOp(bool align_corner, InterpolateOpType op_type);
    InterpolateWithShapeOp(const InterpolateWithShapeOp& op);
    virtual ~InterpolateWithShapeOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int runOnCpu(std::vector<Tensor> input={});
    virtual int runOnGpu(std::vector<Tensor> input={});

private:
    InterpolateOpType m_op_type;
    bool m_with_align;
    int m_align_mode;
};
} // namespace dataflow
} // namespace eagleeye


#endif