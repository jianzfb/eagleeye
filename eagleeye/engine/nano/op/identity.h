#ifndef _EAGLEEYE_EMPTY_H_
#define _EAGLEEYE_EMPTY_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/common/EagleeyeRuntime.h"
#include <vector>
#include "eagleeye/engine/nano/op/FixedCNNOp.h"

namespace eagleeye{
class Identity:public FixedCNNOp{
public:
    Identity(int input_data_num, int output_data_num, std::string op_name);
    virtual ~Identity();

    void run_on_cpu(std::vector<Tensor<float>>& output);
    void run_on_gpu(std::vector<Tensor<float>>& output);

    void run_on_cpu(std::vector<Tensor<float>>& output, std::vector<Tensor<float>>& input);
    void run_on_gpu(std::vector<Tensor<float>>& output, std::vector<Tensor<float>>& input);

private:

};    
}
#endif