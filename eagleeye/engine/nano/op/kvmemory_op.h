#ifndef _EAGLEEYE_KVMEMORY_OP_
#define _EAGLEEYE_KVMEMORY_OP_

#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class KVDMemoryOp;
class KVMemoryOp:public BaseOp<1, 1>, DynamicCreator<KVMemoryOp>{
public:
    friend class KVDMemoryOp;
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
    static std::map<std::string, bool> m_g_is_init;

protected:
    std::string m_memory_name;
    std::string m_cache_folder;
    bool m_is_load_once;
};

class KVDMemoryOp:public BaseOp<2, 1>, DynamicCreator<KVDMemoryOp>{
public:
    using BaseOp<2, 1>::init;
    KVDMemoryOp();
    KVDMemoryOp(const KVDMemoryOp& op);
    virtual ~KVDMemoryOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params);

    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    KVMemoryOp* m_kv_memory_op;
};
}
}

#endif