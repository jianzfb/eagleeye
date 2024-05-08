#ifndef _EAGLEEYE_KVMEMORY_W_OP_
#define _EAGLEEYE_KVMEMORY_W_OP_

#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class KVMemoryWOp:public BaseOp<3, 1>, DynamicCreator<KVMemoryWOp>{
public:
    using BaseOp<3, 1>::init;
    KVMemoryWOp();
    KVMemoryWOp(const KVMemoryWOp& op);
    virtual ~KVMemoryWOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params);

    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

public:
    // key: (16(ID)+16(扩展编码))
    static std::map<std::string, std::map<std::string, Tensor>> m_g_memory;
    static std::map<std::string, std::map<std::string, std::vector<std::string>>> m_g_info;

protected:
    std::string m_memory_name;
    std::string m_cache_folder;
    std::string m_cache_memory_folder;

    bool m_is_init;
    bool m_enable_group;
};
}
}

#endif