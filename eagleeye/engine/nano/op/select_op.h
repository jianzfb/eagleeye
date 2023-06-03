#ifndef _EAGLEEYE_SELECT_OP_
#define _EAGLEEYE_SELECT_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class SelectOp:public BaseOp<2, 1>,DynamicCreator<SelectOp>{
public:
    SelectOp() = default;
    SelectOp(int begin, int end=-1);
    SelectOp(const SelectOp& op);
    virtual ~SelectOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    int64_t m_begin;
    int64_t m_end;
};
} // namespace dataflow
} // namespace eagleeye

#endif