#include "eagleeye/engine/nano/op/kvmemory_r_op.h"
#include "eagleeye/engine/nano/op/kvmemory_w_op.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeIO.h"
#include "eagleeye/common/EagleeyeFile.h"
#include <fstream>

namespace eagleeye{
namespace dataflow{
KVMemoryROp::KVMemoryROp(){
    m_memory_name = "memory";
    m_cache_folder = "./cache";
    m_cache_memory_folder = "";
    m_in_memory = false;
}

KVMemoryROp::KVMemoryROp(const KVMemoryROp& op){
}

KVMemoryROp::~KVMemoryROp(){
}

int KVMemoryROp::init(std::map<std::string, std::vector<std::string>> params){
    if(params.find("name") != params.end()){
        m_memory_name = params["name"][0];
    }
    if(params.find("model_folder") != params.end()){
        m_cache_folder = params["model_folder"][0];
    }
    return 0;
}

int KVMemoryROp::init(std::map<std::string, std::vector<float>> params){
    if(params.find("in_memory") != params.end()){
        m_in_memory = bool(int(params["in_memory"][0]));
    }
    return 0;
}

int KVMemoryROp::runOnCpu(const std::vector<Tensor>& input){
    // input: key
    // output: value
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
    if(input[0].empty() || input[0].dims()[0] == 0){
        this->m_outputs[0] = Tensor();
        return 0;
    }

    const char* key = input[0].cpu<char>();
    int key_size = input[0].dims()[0];
    char* key_str_ptr = (char*)malloc(key_size+1);
    memset(key_str_ptr, '\0', key_size+1);
    key_str_ptr[key_size] = '\0';
    std::string key_str = key_str_ptr;

    if(m_in_memory){
        if(KVMemoryWOp::m_g_memory[m_memory_name].find(key_str) == KVMemoryWOp::m_g_memory[m_memory_name].end()){
            this->m_outputs[0] = Tensor();
            free(key_str_ptr);
            return 0;
        }

        this->m_outputs[0] = KVMemoryWOp::m_g_memory[m_memory_name][key_str];
        free(key_str_ptr);
        return 0;
    }

    std::string key_mem_filename = m_cache_memory_folder + "/" + key_str;
    if(!isfileexist(key_mem_filename.c_str())){
        this->m_outputs[0] = Tensor();
        free(key_str_ptr);
        return 0;
    }

    EagleeyeIO yard_io;
    yard_io.createReadHandle(c_file_path, READ_BINARY_MODE);
    Tensor value;
    yard_io.read(value);
    yard_io.destroyHandle();

    this->m_outputs[0] = value;
    free(key_str_ptr);
    return 0;
}

int KVMemoryROp::runOnGpu(const std::vector<Tensor>& input){
    return -1;
}
}
}