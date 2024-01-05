#ifndef _EAGLEEYE_SWITCH_OP_
#define _EAGLEEYE_SWITCH_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/basic/Dim.h"
#include "eagleeye/basic/type.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
template<std::size_t IN>
class SwitchBase:public BaseOp<IN, 1>{
public:
    using BaseOp<IN, 1>::init;
    SwitchBase(){}
    virtual ~SwitchBase(){};

    virtual int init(std::map<std::string, std::vector<float>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input){
        const int* state_info_ptr = input[0].cpu<int>();
        int state_val = state_info_ptr[0];

        this->m_outputs[0] = input[state_val + 1];
        return 0;
    }
    virtual int runOnGpu(const std::vector<Tensor>& input){return 0;};
};

class Switch2Op:public SwitchBase<3>,DynamicCreator<Switch2Op>{
public:
    using SwitchBase<3>::init;
    Switch2Op(){}
    virtual ~Switch2Op(){};
};
typedef Switch2Op SwitchOp;

class Switch3Op:public SwitchBase<4>,DynamicCreator<Switch3Op>{
public:
    using SwitchBase<4>::init;
    Switch3Op(){}
    virtual ~Switch3Op(){};    
};

class Switch4Op:public SwitchBase<5>,DynamicCreator<Switch4Op>{
public:
    using SwitchBase<5>::init;
    Switch4Op(){}
    virtual ~Switch4Op(){};        
};
}
}
#endif