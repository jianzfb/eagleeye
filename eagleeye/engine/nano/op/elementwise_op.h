#ifndef _EAGLEEYE_ELEMENTWISE_OP_
#define _EAGLEEYE_ELEMENTWISE_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include <vector>

namespace eagleeye{
namespace dataflow{
enum ElementwiseOpType{
    ELEMENTWISE_ADD = 0,
    ELEMENTWISE_SUB = 1,
    ELEMENTWISE_MUL = 2,
    ELEMENTWISE_DIV = 3,
    ELEMENTWISE_POW = 4
}; 
class ElementwiseOp:public BaseOp<Tensor, 2, 1>{
public:
    ElementwiseOp();
    ElementwiseOp(const ElementwiseOp& op);
    virtual ~ElementwiseOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    void processNCHWOnCpu(Tensor x, Tensor y);
    void processNCOnCpu(Tensor x, Tensor y);

    ElementwiseOpType m_op_type;
}; 

class AddOp:public ElementwiseOp{
public:
    AddOp(){
        m_op_type = ELEMENTWISE_ADD;
    }
    virtual ~AddOp(){}
};

class SubOp:public ElementwiseOp{
public:
    SubOp(){
        m_op_type = ELEMENTWISE_SUB;
    }
    virtual ~SubOp(){}
};

class MulOp:public ElementwiseOp{
public:
    MulOp(){
        m_op_type = ELEMENTWISE_MUL;
    }
    virtual ~MulOp(){}
};

class DivOp:public ElementwiseOp{
public:
    DivOp(){
        m_op_type = ELEMENTWISE_DIV;
    }
    virtual ~DivOp(){}
};

class PowOp:public ElementwiseOp{
public:
    PowOp(){
        m_op_type=ELEMENTWISE_POW;
    }
    virtual ~PowOp(){}
};
}
} // namespace eagleeye


#endif