#include "None_plugin.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeLog.h"
#include <iostream>

using namespace eagleeye;

int main(int argc, char** argv){
    // 1.step initialize None module
    const char* config_folder = NULL;   // None module configure folder
    eagleeye_None_initialize(config_folder);
    
    // 2.step set module pipeline parameter
    char* node_name = "";     // NODE NAME in pipeline
    char* param_name = "";    // PARAMETER NAME of NODE in pipeline
    void* value = NULL;       // PARAMETER VALUE
    bool is_ok = eagleeye_None_set_param(node_name, param_name, value);
    if(is_ok){
        EAGLEEYE_LOGD("success to set parameter %s of node %s",param_name,node_name);
    }
    else{
        EAGLEEYE_LOGE("fail to set parameter %s of node %s",param_name,node_name);
    }

    // 3.step set input data 
    void* data = NULL; // YOUR IMAGE DATA POINTER;
    int data_size[] = {0,0,0}; // {IMAGE HEIGHT, IMAGE WIDTH, IMAGE CHANNEL}; 
    is_ok = eagleeye_None_set_input("INPUT NODE", data, data_size, 3);
    if(is_ok){
        EAGLEEYE_LOGD("success to set data for pipeline input node");
    }
    else{
        EAGLEEYE_LOGE("fail to set data for pipeline input node");
    }

    // 4.step refresh module pipeline
    eagleeye_None_run();

    // 5.step get output data of None module
    void* out_data;         // RESULT DATA POINTER
    int out_data_size[3];   // RESULT DATA SHAPE (IMAGE HEIGHT, IMAGE WIDTH, IMAGE CHANNEL)
    int out_data_dims=0;    // 3
    int out_data_type=0;    // RESULT DATA TYPE 
    is_ok = eagleeye_None_get_output("OUTPUT NODE/OUTPUT PORT",out_data, out_data_size, out_data_dims, out_data_type);   
    if(is_ok){
        EAGLEEYE_LOGD("success to get data for pipeline output node");
    }
    else{
        EAGLEEYE_LOGE("fail to set data for pipeline output node");
    }

    // 6.step (optional) sometimes, could call this function to reset all intermedianl state in pipeline
    is_ok = eagleeye_None_reset();
    if(is_ok){
        EAGLEEYE_LOGD("success to reset pipeline");
    }
    else{
        EAGLEEYE_LOGE("fail to reset pipeline");
    }

    // 7.step release None module
    eagleeye_None_release();
}