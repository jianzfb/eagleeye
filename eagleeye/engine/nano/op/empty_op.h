#ifndef _EAGLEEYE_EMPTY_OP_
#define _EAGLEEYE_EMPTY_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/basic/Dim.h"
#include "eagleeye/basic/type.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class Empty1Op:public BaseOp<0,1>, DynamicCreator<Empty1Op>{
public:
    using BaseOp<0,1>::init;
    Empty1Op(){
        this->m_outputs[0] = Tensor(
            std::vector<int64_t>{0},
            EAGLEEYE_FLOAT,
            DataFormat::AUTO,
            CPU_BUFFER
        );
    }
    virtual ~Empty1Op(){

    }

    virtual int init(std::map<std::string, std::vector<float>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}

    virtual int runOnCpu(const std::vector<Tensor>& input){
        return 0;
    }

    virtual int runOnGpu(const std::vector<Tensor>& input){
        return 0;
    }
};

class Empty2Op:public BaseOp<0,2>, DynamicCreator<Empty2Op>{
public:
    using BaseOp<0,2>::init;
    Empty2Op(){
        this->m_outputs[0] = Tensor(
            std::vector<int64_t>{0},
            EAGLEEYE_FLOAT,
            DataFormat::AUTO,
            CPU_BUFFER
        );
        this->m_outputs[1] = Tensor(
            std::vector<int64_t>{0},
            EAGLEEYE_FLOAT,
            DataFormat::AUTO,
            CPU_BUFFER
        );
    }
    virtual ~Empty2Op(){

    }

    virtual int init(std::map<std::string, std::vector<float>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}

    virtual int runOnCpu(const std::vector<Tensor>& input){
        return 0;
    }

    virtual int runOnGpu(const std::vector<Tensor>& input){
        return 0;
    }
};
}
}
#endif