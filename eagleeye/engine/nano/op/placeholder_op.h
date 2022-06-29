#ifndef _EAGLEEYE_PLACEHOLDER_OP_
#define _EAGLEEYE_PLACEHOLDER_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include<vector>

namespace eagleeye{
namespace dataflow{
/**
 * @brief placeholder op
 * 
 */
class PlaceholderOp:public BaseOp<Tensor, 0, 1>{
public:
    PlaceholderOp();    // default constructor
    PlaceholderOp(int64_t b, int64_t h, int64_t w, int64_t c, DataFormat format, EagleeyeType type, MemoryType memory_type=CPU_BUFFER);
    PlaceholderOp(const PlaceholderOp& op);
    virtual ~PlaceholderOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);
    virtual int update(void* data, std::vector<int64_t> shape, int index=0);

private:
    int64_t m_b;
    int64_t m_h;
    int64_t m_w;
    int64_t m_c;
    MemoryType m_memory_type;
    DataFormat m_data_format;
    EagleeyeType m_data_type;
};
}    
}
#endif