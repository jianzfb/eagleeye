#include "eagleeye/engine/nano/op/kvmemory_op.h"
#include "eagleeye/engine/nano/op/kvmemory_w_op.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeIO.h"
#include "eagleeye/common/EagleeyeFile.h"
#include <sys/time.h>
#include <fstream>

namespace eagleeye{
namespace dataflow{
std::map<std::string, std::map<std::string, Tensor>> KVMemoryOp::m_g_memory;
std::map<std::string, std::map<std::string, std::vector<std::string>>> KVMemoryOp::m_g_info;
std::map<std::string, long> KVMemoryOp::m_g_time;
bool KVMemoryOp::m_is_init;

KVMemoryOp::KVMemoryOp(){
    m_memory_name = "memory";
    m_cache_folder = "./cache";
    m_cache_memory_folder = "";
}

KVMemoryOp::KVMemoryOp(const KVMemoryOp& op){
}

KVMemoryOp::~KVMemoryOp(){
}

int KVMemoryOp::init(std::map<std::string, std::vector<std::string>> params){
    if(params.find("name") != params.end()){
        m_memory_name = params["name"][0];
    }
    if(params.find("model_folder") != params.end()){
        m_cache_folder = params["model_folder"][0];
    }
    return 0;
}

int KVMemoryOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

int KVMemoryOp::runOnCpu(const std::vector<Tensor>& input){
    // input: nothing(0)/update(1)/delete(2)
    if(m_cache_memory_folder == ""){
        if(endswith(m_cache_folder, "/")){
           m_cache_memory_folder = m_cache_folder + m_memory_name;
        }
        else{
            m_cache_memory_folder = m_cache_folder + "/" + m_memory_name;
        }
    }
    if(!isdirexist(m_cache_memory_folder.c_str())){
        createdirectory(m_cache_memory_folder.c_str());
    }

    if(m_outputs[0].empty()){
        this->m_outputs[0] = Tensor(
            std::vector<int64_t>{int64_t(m_memory_name.size())},
            EAGLEEYE_UCHAR,
            DataFormat::AUTO,
            CPU_BUFFER
        );

        memcpy(this->m_outputs[0].cpu<char>(), m_memory_name.c_str(), m_memory_name.size());
    }

    int operator_status = input[0].cpu<int>()[0];
    if(!m_is_init || operator_status==1){
        // 将缓存文件加载到内存
        std::vector<std::string> file_list;
        traverseFiles(m_cache_memory_folder.c_str(), file_list);
        for(int key_i = 0; key_i < file_list.size(); ++key_i){
            std::string key_name = file_list[key_i];
            std::string key_file_path = m_cache_memory_folder + "/" + key_name;
            EagleeyeIO yard_io;
            yard_io.createReadHandle(key_file_path, READ_BINARY_MODE);
            Tensor value;
            yard_io.read(value);
            yard_io.destroyHandle();
            KVMemoryOp::m_g_memory[m_memory_name][key_name] = value;

            std::string group_name = key_name.substr(0, 16);
            KVMemoryOp::m_g_info[m_memory_name][group_name].push_back(key_name);
        }

        struct timeval timestamp;
        gettimeofday(&timestamp,NULL);
        KVMemoryOp::m_g_time[m_memory_name] = timestamp.tv_sec * 1000000 + timestamp.tv_usec;
        m_is_init = true;
    }

    if(operator_status == 0 || operator_status == 1){
        return 0;
    }

    if(KVMemoryOp::m_g_memory.find(m_memory_name) != KVMemoryOp::m_g_memory.end()){
        // 清理文件
        std::map<std::string, std::vector<std::string>>::iterator iter, iend(KVMemoryOp::m_g_info[m_memory_name].end());
        for(iter = KVMemoryOp::m_g_info[m_memory_name].begin(); iter != iend; ++iter){
            for(int i=0; i<iter->second.size(); ++i){
                std::string key_file_path = m_cache_memory_folder + "/" + iter->second[i];
                deletefile(key_file_path.c_str());
            }
        }

        // 清理内存
        KVMemoryOp::m_g_memory.erase(m_memory_name);
        KVMemoryOp::m_g_info.erase(m_memory_name);
        KVMemoryOp::m_g_time.erase(m_memory_name);
    }
    return 0;
}

int KVMemoryOp::runOnGpu(const std::vector<Tensor>& input){
    return -1;
}
}
}