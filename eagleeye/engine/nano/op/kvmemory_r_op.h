#ifndef _EAGLEEYE_KVMEMORY_R_OP_
#define _EAGLEEYE_KVMEMORY_R_OP_

#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class KVMemoryROp:public BaseOp<1, 1>, DynamicCreator<KVMemoryROp>{
public:
    using BaseOp<1, 1>::init;
    KVMemoryROp();
    KVMemoryROp(const KVMemoryROp& op);
    virtual ~KVMemoryROp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params);

    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    std::string m_memory_name;
    std::string m_cache_folder;
    std::string m_cache_memory_folder;
    bool m_in_memory;
};
}
}

#endif