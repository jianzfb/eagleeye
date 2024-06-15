#ifndef _EAGLEEYE_KVMEMORY_OP_
#define _EAGLEEYE_KVMEMORY_OP_

#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class KVMemoryOp:public BaseOp<1, 1>, DynamicCreator<KVMemoryOp>{
public:
    using BaseOp<1, 1>::init;
    KVMemoryOp();
    KVMemoryOp(const KVMemoryOp& op);
    virtual ~KVMemoryOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params);

    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

public:
    // key: (16(ID)+16(扩展编码))
    static std::map<std::string, std::map<std::string, Tensor>> m_g_memory;
    static std::map<std::string, std::map<std::string, std::vector<std::string>>> m_g_info;
    static std::map<std::string, long> m_g_time;
    static bool m_is_init;

protected:
    std::string m_memory_name;
    std::string m_cache_folder;
    std::string m_cache_memory_folder;

};
}
}

#endif