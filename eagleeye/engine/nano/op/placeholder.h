#ifndef _EAGLEEYE_PLACEHOLDER_
#define _EAGLEEYE_PLACEHOLDER_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/util/opencl_util.h"
#include<vector>
#include "eagleeye/common/EagleeyeOpenCL.h"

namespace eagleeye{
namespace dataflow{
/**
 * @brief placeholder op
 * 
 */
class Placeholder:public BaseOp<Tensor, 0, 1>{
public:
    Placeholder(std::string name);
    virtual ~Placeholder();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int runOnCpu(std::vector<Tensor> input=std::vector<Tensor>{});
    virtual int runOnGpu(std::vector<Tensor> input=std::vector<Tensor>{});
    virtual int update(void* data, int index=0);

private:
    int64_t m_b;
    int64_t m_h;
    int64_t m_w;
    int64_t m_c;
    MemoryType m_memory_type;
    DataFormat m_data_format;
    EagleeyeType m_data_type;

    std::string m_name;
};
}    
}
#endif