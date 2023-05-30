#ifndef _EAGLEEYE_MESHGRID_OP_
#define _EAGLEEYE_MESHGRID_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class MeshgridOp: public BaseOp<Tensor, 2, 2>{
public:
    MeshgridOp() = default;
    virtual ~MeshgridOp(){};

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:

};
} // namespace dataflow
} // namespace eagleeye

#endif