#include "eagleeye/engine/nano/op/kvmemory_r_op.h"
#include "eagleeye/engine/nano/op/kvmemory_op.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeIO.h"
#include "eagleeye/common/EagleeyeFile.h"
#include <stdio.h>
#include <fstream>

namespace eagleeye{
namespace dataflow{
KVMemoryROp::KVMemoryROp(){
}

KVMemoryROp::KVMemoryROp(const KVMemoryROp& op){
}

KVMemoryROp::~KVMemoryROp(){
}

int KVMemoryROp::init(std::map<std::string, std::vector<std::string>> params){
    return 0;
}

int KVMemoryROp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

int KVMemoryROp::runOnCpu(const std::vector<Tensor>& input){
    // input: memory, key
    // output: value
    if(input[1].empty() || input[1].dims()[0] == 0 || (input[1].dims()[1] != 16 && input[1].dims()[1] != 32)){
        this->m_outputs[0] = Tensor();
        return 0;
    }

    // memory name
    const char* memory_name_ptr = input[0].cpu<char>();
    char* memory_name_str_ptr = (char*)malloc(input[0].dims().production() + 1);
    memset(memory_name_str_ptr, '\0', input[0].dims().production() + 1);
    memcpy(memory_name_str_ptr, memory_name_ptr, input[0].dims().production());
    std::string memory_name = memory_name_str_ptr;
    free(memory_name_str_ptr);

    if(KVMemoryOp::m_g_memory.find(memory_name) == KVMemoryOp::m_g_memory.end()){
        this->m_outputs[0] = Tensor();
        return 0;
    }

    // key
    const char* key = input[1].cpu<char>();
    int key_size = input[1].dims()[1];
    char* key_str_ptr = (char*)malloc(key_size+1);
    memset(key_str_ptr, '\0', key_size+1);
    memcpy(key_str_ptr, key, key_size);
    std::string key_str = key_str_ptr;
    free(key_str_ptr);

    if(key_size == 16){
        // 匹配，返回
        std::vector<Tensor> tensor_list;
        std::map<std::string, Tensor>::iterator iter, iend(KVMemoryOp::m_g_memory[memory_name].end());
        for(iter=KVMemoryOp::m_g_memory[memory_name].begin(); iter!=iend; ++iter){
            if(strncmp(iter->first.c_str(), key_str.c_str(), 16) == 0){
                tensor_list.push_back(iter->second);
            }
        }
        if(tensor_list.size() == 0){
            this->m_outputs[0] = Tensor();
            return 0;
        }
        int64_t tensor_n = tensor_list.size();
        int64_t feature_dim = tensor_list[0].dims().production();
        std::vector<int64_t> stack_tensor_shape = {tensor_n};
        for(int i=0; i<tensor_list[0].dims().size(); ++i){
            stack_tensor_shape.push_back(tensor_list[0].dims()[i]);
        }

        Tensor stack_tensor(
            stack_tensor_shape,
            tensor_list[0].type(),
            DataFormat::AUTO,
            CPU_BUFFER
        );
        for(int tensor_i=0; tensor_i<tensor_list.size(); ++tensor_i){
            char* offset_ptr = stack_tensor.cpu<char>() + tensor_i * feature_dim * stack_tensor.elemsize();
            char* src_ptr = tensor_list[tensor_i].cpu<char>();
            memcpy(offset_ptr, src_ptr, sizeof(char)*feature_dim*stack_tensor.elemsize());
        }
        this->m_outputs[0] = stack_tensor;
    }
    else{
        // 精确匹配
        if(KVMemoryOp::m_g_memory[memory_name].find(key_str) == KVMemoryOp::m_g_memory[memory_name].end()){
            this->m_outputs[0] = Tensor();
            return 0;
        }

        this->m_outputs[0] = KVMemoryOp::m_g_memory[memory_name][key_str];
    }
    return 0;
}

int KVMemoryROp::runOnGpu(const std::vector<Tensor>& input){
    return -1;
}
}
}