#ifndef _EAGLEEYE_KVMEMORY_W_OP_
#define _EAGLEEYE_KVMEMORY_W_OP_

#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class KVMemoryWOp:public BaseOp<4, 1>, DynamicCreator<KVMemoryWOp>{
public:
    using BaseOp<4, 1>::init;
    KVMemoryWOp();
    KVMemoryWOp(const KVMemoryWOp& op);
    virtual ~KVMemoryWOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params);

    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    std::string m_cache_folder;
    std::string m_cache_memory_folder;
};
}
}

#endif