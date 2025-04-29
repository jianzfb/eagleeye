#ifndef _EAGLEEYE_KVMEMORY_H_
#define _EAGLEEYE_KVMEMORY_H_
#include <mutex>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include "eagleeye/common/CJsonObject.hpp"


namespace eagleeye{
class KVMemoryManage{
public:
    KVMemoryManage(std::string root_folder, std::string memory_name);
    virtual ~KVMemoryManage();

    long insert(std::string key, std::vector<float> value, int& index);
    long remove(int index);
    long save();

    long load(std::function<void(neb::CJsonObject&)> callback);
    long time();

private:
    std::string m_root_folder;
    std::string m_memory_name;
    std::string m_memory_file_path;
    long m_save_time;
    neb::CJsonObject m_content;
};
}
#endif