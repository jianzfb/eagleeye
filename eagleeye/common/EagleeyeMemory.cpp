#include "eagleeye/common/EagleeyeMemory.h"
#include "eagleeye/common/EagleeyeFile.h"
#include "eagleeye/common/EagleeyeStr.h"
#include <fstream>
#include <cstring>
#include <sys/time.h>

namespace eagleeye{
std::map<std::string, long> g_memory_update_time;
std::mutex g_memory_mu;


KVMemoryManage::KVMemoryManage(std::string root_folder, std::string memory_name){
    this->m_root_folder = root_folder;
    this->m_memory_name = memory_name;
    if(endswith(m_root_folder, "/")){
        this->m_memory_file_path = m_root_folder + m_memory_name+".json";
    }
    else{
        this->m_memory_file_path  = m_root_folder + "/" + m_memory_name+".json";
    }

    struct timeval now_tv;
    gettimeofday(&now_tv,NULL);
    m_save_time = 0;
    g_memory_update_time[m_memory_name] = 0;
    if(isfileexist(m_memory_file_path.c_str())){
        m_save_time = now_tv.tv_sec * 1000000 + now_tv.tv_usec;
        g_memory_update_time[m_memory_name] = m_save_time;

        std::ifstream i_file_handle;
        i_file_handle.open(this->m_memory_file_path);
        i_file_handle.seekg(0, std::ios::end);
        int file_buffer_size = i_file_handle.tellg();
        i_file_handle.seekg(0, std::ios::beg);
        char* file_buffer = new char[file_buffer_size+1];
        memset(file_buffer, '\0', sizeof(char)*(file_buffer_size+1));
        i_file_handle.read(file_buffer, file_buffer_size);
        std::string db_content = file_buffer;
        i_file_handle.close();
        delete[] file_buffer;
        m_content = neb::CJsonObject(db_content);
    }
}

KVMemoryManage::~KVMemoryManage(){
    std::unique_lock<std::mutex> memory_locker(g_memory_mu);
    if(g_memory_update_time.find(this->m_memory_name) == g_memory_update_time.end()){
        return;
    }

    if(m_save_time < g_memory_update_time[this->m_memory_name]){
        std::ofstream out_f;
        out_f.open(this->m_memory_file_path);
        out_f << m_content.ToString();
        out_f.close();
    }
}

long KVMemoryManage::insert(std::string key, std::vector<float> value, int& index){
    std::unique_lock<std::mutex> memory_locker(g_memory_mu);
    if(g_memory_update_time.find(this->m_memory_name) == g_memory_update_time.end()){
        return -1;
    }
    if(value.size() == 0){
        return -1;
    }

    neb::CJsonObject kv;
    neb::CJsonObject obj;
    for(int i=0; i<value.size(); ++i){
        obj.Add(value[i]);
    }
    kv.Add("value", obj);
    kv.Add("key", key);

    m_content.Add(kv);
    index = m_content.GetArraySize() - 1;

    struct timeval now_tv;
    gettimeofday(&now_tv,NULL);
    g_memory_update_time[this->m_memory_name] = now_tv.tv_sec * 1000000 + now_tv.tv_usec;
    return g_memory_update_time[this->m_memory_name];
}

long KVMemoryManage::remove(int index){
    std::unique_lock<std::mutex> memory_locker(g_memory_mu);
    if(g_memory_update_time.find(this->m_memory_name) == g_memory_update_time.end()){
        return -1;
    }

    m_content.Delete(index);

    struct timeval now_tv;
    gettimeofday(&now_tv,NULL);
    g_memory_update_time[this->m_memory_name] = now_tv.tv_sec * 1000000 + now_tv.tv_usec;
    return g_memory_update_time[this->m_memory_name];
}

long KVMemoryManage::save(){
    std::unique_lock<std::mutex> memory_locker(g_memory_mu);
    if(g_memory_update_time.find(this->m_memory_name) == g_memory_update_time.end()){
        return -1;
    }
    if(m_save_time < g_memory_update_time[this->m_memory_name]){
        std::ofstream out_f;
        out_f.open(this->m_memory_file_path);
        out_f << m_content.ToString();
        out_f.close();
        m_save_time = g_memory_update_time[this->m_memory_name];
    }

    return m_save_time;
}

long KVMemoryManage::load(std::function<void(neb::CJsonObject&)> callback){
    std::unique_lock<std::mutex> memory_locker(g_memory_mu);
    if(g_memory_update_time[m_memory_name] == 0){
        return g_memory_update_time[m_memory_name];
    }

    // 已经存在数据，调用用户回调
    callback(m_content);
    return g_memory_update_time[this->m_memory_name];
}

long KVMemoryManage::time(){
    std::unique_lock<std::mutex> memory_locker(g_memory_mu);
    return g_memory_update_time[this->m_memory_name];
}
}