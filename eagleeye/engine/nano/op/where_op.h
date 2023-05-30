#ifndef _EAGLEEYE_WHERE_OP_
#define _EAGLEEYE_WHERE_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class WhereOp: public BaseOp<Tensor, 3, 1>{
public:
    WhereOp();
    WhereOp(const WhereOp& op);

    virtual ~WhereOp(){}

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:

};
}    
}
#endif