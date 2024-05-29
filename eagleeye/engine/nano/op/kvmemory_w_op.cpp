#include "eagleeye/engine/nano/op/kvmemory_w_op.h"
#include "eagleeye/engine/nano/op/kvmemory_op.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeIO.h"
#include "eagleeye/common/EagleeyeFile.h"
#include <sys/time.h>
#include <fstream>

namespace eagleeye{
namespace dataflow{
KVMemoryWOp::KVMemoryWOp(){
    m_cache_folder = "./cache";
    m_cache_memory_folder = "";
}

KVMemoryWOp::KVMemoryWOp(const KVMemoryWOp& op){
}

KVMemoryWOp::~KVMemoryWOp(){
}

int KVMemoryWOp::init(std::map<std::string, std::vector<std::string>> params){
    if(params.find("model_folder") != params.end()){
        m_cache_folder = params["model_folder"][0];
    }
    return 0;
}

int KVMemoryWOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

int KVMemoryWOp::runOnCpu(const std::vector<Tensor>& input){
    // input: memory_name, nothing(0)/add(1)/del(2), key, value
    // output: key
    const char* memory_name_ptr = input[0].cpu<char>();
    char* memory_name_str_ptr = (char*)malloc(input[0].dims().production() + 1);
    memset(memory_name_str_ptr, '\0', input[0].dims().production() + 1);
    memcpy(memory_name_str_ptr, memory_name_ptr, input[0].dims().production());
    std::string memory_name = memory_name_str_ptr;
    free(memory_name_str_ptr);

    if(m_cache_memory_folder == ""){
        if(endswith(m_cache_folder, "/")){
           m_cache_memory_folder = m_cache_folder + memory_name;
        }
        else{
            m_cache_memory_folder = m_cache_folder + "/" + memory_name;
        }
    }
    if(!isdirexist(m_cache_memory_folder.c_str())){
        createdirectory(m_cache_memory_folder.c_str());
    }

    if(input[3].empty() || input[3].dims()[0] == 0){
        // 非有效记录，直接跳过
        // do nothing
        this->m_outputs[0] = Tensor(
            std::vector<int64_t>{0},
            EAGLEEYE_UCHAR,
            DataFormat::AUTO,
            CPU_BUFFER
        );
        return 0;
    }

    struct timeval now_tv;
    gettimeofday(&now_tv,NULL);
    int operator_status = input[1].cpu<int>()[0];
    if(operator_status == 0){
        // 无效操作，直接跳过
        // do nothing
        this->m_outputs[0] = Tensor(
            std::vector<int64_t>{0},
            EAGLEEYE_UCHAR,
            DataFormat::AUTO,
            CPU_BUFFER
        );
        return 0;
    }

    if(operator_status == 1){
        // add
        std::string group_str;
        if(input[2].empty() || input[2].dims()[0] == 0 || (input[2].dims()[1] != 16 && input[2].dims()[1] != 32)){
            // 动态生成唯一编码
            group_str = std::to_string(now_tv.tv_sec * 1000000 + now_tv.tv_usec);
        }
        else{
            // 静态编码(16位地址)
            const char* key = input[2].cpu<char>();
            int key_size = input[2].dims()[1];
            key_size = key_size <= 16 ?  key_size : 16;

            char* group_str_ptr = (char*)malloc(16+1);
            memset(group_str_ptr, '0', 16+1);
            memcpy(group_str_ptr, key, key_size);
            group_str_ptr[16] = '\0';
            group_str = group_str_ptr;
            free(group_str_ptr);
        }

        // 扩展编码(16位地址)
        std::string ext_str = std::to_string(now_tv.tv_sec * 1000000 + now_tv.tv_usec);
        std::string key_str = group_str + ext_str;
        std::string key_mem_filename = m_cache_memory_folder + "/" + key_str;
        KVMemoryOp::m_g_memory[memory_name][key_str] = input[3];
        KVMemoryOp::m_g_info[memory_name][group_str].push_back(key_str);
        KVMemoryOp::m_g_time[memory_name] = now_tv.tv_sec * 1000000 + now_tv.tv_usec;

        EagleeyeIO yard_io;
        yard_io.createWriteHandle(key_mem_filename.c_str(), false, WRITE_BINARY_MODE);
        yard_io.write(input[3]);
        yard_io.destroyHandle();

        this->m_outputs[0] = Tensor(
            std::vector<int64_t>{(int64_t)(key_str.size())},
            EAGLEEYE_UCHAR,
            DataFormat::AUTO,
            CPU_BUFFER
        );
        memcpy(this->m_outputs[0].cpu<char>(), key_str.c_str(), key_str.size());
    }
    else{
        // del
        const char* key = input[2].cpu<char>();
        int key_size = input[2].dims()[1];
        char* key_str_ptr = (char*)malloc(key_size+1);
        memset(key_str_ptr, '\0', key_size+1);
        memcpy(key_str_ptr, key, key_size);
        std::string key_str = key_str_ptr;
        std::string key_mem_filename = m_cache_memory_folder + "/" + key_str;

        std::string group_str = key_str.substr(0, 16);
        if(KVMemoryOp::m_g_memory[memory_name].find(key_str) != KVMemoryOp::m_g_memory[memory_name].end()){
            KVMemoryOp::m_g_memory[memory_name].erase(key_str);

            std::vector<std::string> update_list;
            for(int i=0; i<KVMemoryOp::m_g_info[memory_name][group_str].size(); ++i){
                if(KVMemoryOp::m_g_info[memory_name][group_str][i] != key_str){
                    update_list.push_back(KVMemoryOp::m_g_info[memory_name][group_str][i]);
                }
            }
            KVMemoryOp::m_g_info[memory_name][group_str] = update_list;
            KVMemoryOp::m_g_time[memory_name] = now_tv.tv_sec * 1000000 + now_tv.tv_usec;
        }

        if(isfileexist(key_mem_filename.c_str())){
            deletefile(key_mem_filename.c_str());
        }
        free(key_str_ptr);

        this->m_outputs[0] = Tensor(
            std::vector<int64_t>{(int64_t)(key_str.size())},
            EAGLEEYE_UCHAR,
            DataFormat::AUTO,
            CPU_BUFFER
        );
        memcpy(this->m_outputs[0].cpu<char>(), key_str.c_str(), key_str.size()); 
    }
    return 0;
}

int KVMemoryWOp::runOnGpu(const std::vector<Tensor>& input){
    return -1;
}
}
}