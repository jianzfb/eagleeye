#ifndef _EAGLEEYE_PAD2D_OP_
#define _EAGLEEYE_PAD2D_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{

enum Pad2dOpType{
    PAD2D_CONSTANT  = 0,
    PAD2D_EDGE      = 1,
    PAD2D_REFLECT   = 3
};

class Pad2dOp:public BaseOp<1, 1>,DynamicCreator<Pad2dOp>{
public:
    using BaseOp<1, 1>::init;
    Pad2dOp(Pad2dOpType pad_type, std::vector<int64_t> pad_c, std::vector<int64_t> pad_h, std::vector<int64_t> pad_w, float pad_value=0.0f);
    Pad2dOp(const Pad2dOp& op);
    virtual ~Pad2dOp();

    Pad2dOp() = default;
    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    float m_pad_value;
    Pad2dOpType m_pad_type;
    std::vector<int64_t> m_pad_h;
    std::vector<int64_t> m_pad_w;
    std::vector<int64_t> m_pad_c;
};

class ConstPad2dOp: public Pad2dOp{
public:
    ConstPad2dOp(std::vector<int64_t> pad_c={}, 
                    std::vector<int64_t> pad_h={}, 
                    std::vector<int64_t> pad_w={}, 
                    float pad_value=0.0f):Pad2dOp(PAD2D_CONSTANT, pad_c, pad_h, pad_w, pad_value){};
    virtual ~ConstPad2dOp(){}
};

class EdgePad2dOp: public Pad2dOp{
public:
    EdgePad2dOp(std::vector<int64_t> pad_c={}, 
                    std::vector<int64_t> pad_h={}, 
                    std::vector<int64_t> pad_w={}, 
                    float pad_value=0.0f):Pad2dOp(PAD2D_EDGE, pad_c, pad_h, pad_w, pad_value){};
    virtual ~EdgePad2dOp(){}
};

class ReflectPad2dOp: public Pad2dOp{
public:
    ReflectPad2dOp(std::vector<int64_t> pad_c={}, 
                    std::vector<int64_t> pad_h={}, 
                    std::vector<int64_t> pad_w={}, 
                    float pad_value=0.0f):Pad2dOp(PAD2D_REFLECT, pad_c, pad_h, pad_w, pad_value){};
    virtual ~ReflectPad2dOp(){}
};
} // namespace dataflow
} // namespace eagleeye



#endif