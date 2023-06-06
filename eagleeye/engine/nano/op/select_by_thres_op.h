#ifndef _EAGLEEYE_SELECT_BY_THRES_OP_
#define _EAGLEEYE_SELECT_BY_THRES_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class SelectByThresOp: public BaseOp<1, 2>,DynamicCreator<SelectByThresOp>{
public:
    SelectByThresOp() = default;
    SelectByThresOp(std::vector<float> thres, int64_t index, bool attach_index=false, bool exclusive=false);  
    SelectByThresOp(const SelectByThresOp& op);

    virtual ~SelectByThresOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    std::vector<float> m_thres;
    int64_t m_index;
    bool m_exclusive;
    bool m_attach_index;
};
} // namespace dataflow
} // namespace eagleeye
#endif