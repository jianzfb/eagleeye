#ifndef _EAGLEEYE_PLACEHOLDER_H_
#define _EAGLEEYE_PLACEHOLDER_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/common/EagleeyeRuntime.h"
#include "eagleeye/engine/nano/op/FixedCNNOp.h"

namespace eagleeye{
class Placeholder:public FixedCNNOp{
public:
    Placeholder(std::string op_name);
    virtual ~Placeholder();

    void run_on_cpu(std::vector<Tensor<FixedType>>& output);
    void run_on_gpu(std::vector<Tensor<FixedType>>& output);
    void run_on_dsp(std::vector<Tensor<FixedType>>& output);

    virtual bool init(char *buf, int in_size);
    virtual int setShape(std::vector<int64_t> shape);

    /**
     * @brief Set the Input data for placeholder
     * 
     * @param data 
     */
    virtual void setInput(std::vector<Tensor<float>>& data);

private:
    float m_min_val;
    float m_max_val;

};

}
#endif