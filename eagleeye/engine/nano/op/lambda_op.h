#ifndef _EAGLEEYE_LAMBDA_OP_
#define _EAGLEEYE_LAMBDA_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{

template<std::size_t IN, std::size_t OUT>
class LambdaOp:public BaseOp<IN, OUT>{
public:
    LambdaOp() = default;
    virtual ~LambdaOp()=default;

    void setCpuProcess(std::function<int(const std::vector<Tensor>&, std::vector<Tensor>&)> func){
        m_cpu_process = func;
    }
    void setGpuProcess(std::function<int(const std::vector<Tensor>&, std::vector<Tensor>&)> func){
        m_gpu_process = func;
    }
    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    std::function<int(const std::vector<Tensor>&, std::vector<Tensor>&)> m_cpu_process;
    std::function<int(const std::vector<Tensor>&, std::vector<Tensor>&)> m_gpu_process;
}; 

} // namespace dataflow
} // namespace eagleeye

#include "eagleeye/engine/nano/op/lambda_op.hpp"
#endif