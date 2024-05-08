#include "eagleeye/engine/nano/op/kvmemory_w_op.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeIO.h"
#include "eagleeye/common/EagleeyeFile.h"
#include <sys/time.h>
#include <fstream>

namespace eagleeye{
namespace dataflow{
std::map<std::string, std::map<std::string, Tensor>> KVMemoryWOp::m_g_memory;
std::map<std::string, std::map<std::string, std::vector<std::string>>> KVMemoryWOp::m_g_info;

KVMemoryWOp::KVMemoryWOp(){
    m_memory_name = "memory";
    m_cache_folder = "./cache";
    m_cache_memory_folder = "";
    m_is_init = false;
    m_enable_group = true;
}

KVMemoryWOp::KVMemoryWOp(const KVMemoryWOp& op){
}

KVMemoryWOp::~KVMemoryWOp(){
}

int KVMemoryWOp::init(std::map<std::string, std::vector<std::string>> params){
    if(params.find("name") != params.end()){
        m_memory_name = params["name"][0];
    }
    if(params.find("model_folder") != params.end()){
        m_cache_folder = params["model_folder"][0];
    }
    return 0;
}

int KVMemoryWOp::init(std::map<std::string, std::vector<float>> params){
    if(params.find("enable_group") != params.end()){
        m_enable_group = bool(int(params["enable_group"][0]));
    }
    return 0;
}

int KVMemoryWOp::runOnCpu(const std::vector<Tensor>& input){
    // input: add(0)/del(1), key, value
    // output: key
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

    if(!m_is_init){
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
            KVMemoryWOp::m_g_memory[m_memory_name][key_name] = value;

            std::string group_name = key_name.substr(0, 16);
            KVMemoryWOp::m_g_info[m_memory_name][group_name].push_back(key_name);
        }
        m_is_init = true;
    }

    if(input[0].empty() || input[0].dims()[0] == 0){
        this->m_outputs[0] = Tensor(
            std::vector<int64_t>{0},
            EAGLEEYE_UCHAR,
            DataFormat::AUTO,
            CPU_BUFFER
        );
        return 0;
    }

    int operator_status = input[0].cpu<int>()[0];
    if(operator_status == 0){
        // add
        std::string group_str;
        if(input[1].empty() || input[1].dims()[0] == 0 || (input[1].dims()[1] != 16 && input[1].dims()[1] != 32)){
            // 动态生成唯一编码
            struct timeval now_tv;
            gettimeofday(&now_tv,NULL);
            group_str = std::to_string(now_tv.tv_sec * 1000000 + now_tv.tv_usec);
        }
        else{
            // 使用指定编码
            const char* key = input[1].cpu<char>();
            int key_size = input[1].dims()[1];
            key_size = key_size <= 16 ?  key_size : 16;

            char* group_str_ptr = (char*)malloc(16+1);
            memset(group_str_ptr, '0', 16+1);
            memcpy(group_str_ptr, key, key_size);
            group_str_ptr[16] = '\0';
            group_str = group_str_ptr;
            free(group_str_ptr);
        }

        std::string ext_str = "";
        if(m_enable_group){
            struct timeval tv;
            gettimeofday(&tv,NULL);
            ext_str = std::to_string(tv.tv_sec * 1000000 + tv.tv_usec);
        }

        std::string key_str = group_str + ext_str;
        std::string key_mem_filename = m_cache_memory_folder + "/" + key_str;
        KVMemoryWOp::m_g_memory[m_memory_name][key_str] = input[2];
        KVMemoryWOp::m_g_info[m_memory_name][group_str].push_back(key_str);

        EagleeyeIO yard_io;
        yard_io.createWriteHandle(key_mem_filename.c_str(), false, WRITE_BINARY_MODE);
        yard_io.write(input[2]);
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
        const char* key = input[1].cpu<char>();
        int key_size = input[1].dims()[1];
        char* key_str_ptr = (char*)malloc(key_size+1);
        memset(key_str_ptr, '\0', key_size+1);
        memcpy(key_str_ptr, key, key_size);
        std::string key_str = key_str_ptr;
        std::string key_mem_filename = m_cache_memory_folder + "/" + key_str;

        std::string group_str = key_str.substr(0, 16);
        if(KVMemoryWOp::m_g_memory[m_memory_name].find(key_str) != KVMemoryWOp::m_g_memory[m_memory_name].end()){
            KVMemoryWOp::m_g_memory[m_memory_name].erase(key_str);

            std::vector<std::string> update_list;
            for(int i=0; i<KVMemoryWOp::m_g_info[m_memory_name][group_str].size(); ++i){
                if(KVMemoryWOp::m_g_info[m_memory_name][group_str][i] != key_str){
                    update_list.push_back(KVMemoryWOp::m_g_info[m_memory_name][group_str][i]);
                }
            }
            KVMemoryWOp::m_g_info[m_memory_name][group_str] = update_list;
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