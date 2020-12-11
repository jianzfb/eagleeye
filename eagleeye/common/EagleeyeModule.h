#ifndef _EAGLEEYE_MODULE_H_
#define _EAGLEEYE_MODULE_H_
#include <string>
#include <vector>
namespace eagleeye{
/**
 * @brief pipeline initialize
 * 
 */
bool eagleeye_pipeline_initialize(const char* pipeline_name, const char* config_folder=NULL);

/**
 * @brief load pipeline config parameter from config file
 * 
 * @param pipeline_name 
 * @param config_file 
 * @return true 
 * @return false 
 */
bool eagleeye_pipeline_load_config(const char* pipeline_name, const char* config_file);

/**
 * @brief save pipeline config parameter to config file
 * 
 * @param pipeline_name 
 * @param config_file 
 * @return true 
 * @return false 
 */
bool eagleeye_pipeline_save_config(const char* pipeline_name, const char* config_file);

/**
 * @brief get all registed pipelines
 * 
 * @param pipeline_names 
 * @return true 
 * @return false 
 */
bool eagleeye_init_module(std::vector<std::string>& pipeline_names, const char* plugin_folder);

/**
 * @brief release module
 * 
 * @return true 
 * @return false 
 */
bool eagleeye_release_module();

/**
 * @brief register pipeline plugin
 * 
 * @param pipeline 
 * @return true 
 * @return false 
 */
bool eagleeye_register_pipeline(const char* pipeline, const char* version, const char* key);

/**
 * @brief set pipeline parameter
 * 
 * @param node_name 
 * @param param_name 
 * @param value 
 */
bool eagleeye_pipeline_set_param(const char* pipeline_name,
                                 const char* node_name, 
                                 const char* param_name, 
                                 const void* value);

/**
 * @brief get pipeline parameter
 * 
 * @param node_name 
 * @param param_name 
 * @param value 
 */
bool eagleeye_pipeline_get_param(const char* pipeline_name,
                                 const char* node_name, 
                                 const char* param_name, 
                                 void* value);

/**
 * @brief set pipeline input
 * 
 * @param node_name 
 * @param data 
 * @param data_size 
 * @param data_dims 
 * @param data_type
 */
bool eagleeye_pipeline_set_input(const char* pipeline_name,
                                 const char* node_name, 
                                 void* data, 
                                 const int* data_size, 
                                 const int data_dims,
                                 const int data_type);

/**
 * @brief get pipeline node output
 * 
 * @param pipeline_name 
 * @param node_name 
 * @param data 
 * @param data_size 
 * @param data_dims 
 * @param data_type 
 * @return true 
 * @return false 
 */
bool eagleeye_pipeline_get_node_output(const char* pipeline_name,
                                  const char* node_name, 
                                  void*& data, 
                                  int* data_size, 
                                  int& data_dims,
                                  int& data_type);

/**
 * @brief get pipeline output
 * 
 * @param node_name 
 * @param data 
 * @param data_size 
 * @param data_dims 
 */
bool eagleeye_pipeline_get_output(const char* pipeline_name,
                                  const char* node_name, 
                                  void*& data, 
                                  int* data_size, 
                                  int& data_dims,
                                  int& data_type);

/**
 * @brief get pipeline monitors config
 * 
 * @param monitor_names 
 * @param monitor_types 
 */
bool eagleeye_pipeline_get_monitor_config(const char* pipeline_name,
                                          std::vector<std::string>& monitor_names,
                                          std::vector<int>& monitor_types,
                                          std::vector<std::string>& monitor_range);

/**
 * @brief get pipeline input config
 * 
 * @param input_nodes 
 * @param input_types 
 * @param input_sources 
 */
bool eagleeye_pipeline_get_input_config(const char* pipeline_name,
                                        std::vector<std::string>& input_nodes,
                                        std::vector<std::string>& input_types,
                                        std::vector<std::string>& input_sources);

/**
 * @brief get pipeline output config
 * 
 * @param output_nodes 
 * @param output_types 
 * @param output_targets 
 */
bool eagleeye_pipeline_get_output_config(const char* pipeline_name,
                                         std::vector<std::string>& output_nodes,
                                         std::vector<std::string>& output_types,
                                         std::vector<std::string>& output_targets);
 
/**
 * @brief isready pipeline
 * 
 * @param pipeline_name 
 * @return true 
 * @return false 
 */
bool eagleeye_pipeline_isready(const char* pipeline_name, int& is_ready);

/**
 * @brief run pipeline
 * 
 */
bool eagleeye_pipeline_run(const char* pipeline_name);

/**
 * @brief reset pipeline
 * 
 * @return true 
 * @return false 
 */
bool eagleeye_pipeline_reset(const char* pipeline_name);

/**
 * @brief release pipeline
 * 
 * @param pipeline_name 
 * @return true 
 * @return false 
 */
bool eagleeye_pipeline_release(const char* pipeline_name);

/**
 * @brief get pipeline version
 * 
 * @param pipeline_name 
 * @param pipeline_version 
 * @return true 
 * @return false 
 */
bool eagleeye_pipeline_version(const char* pipeline_name, char* pipeline_version);


/**
 * @brief replace (debug pipeline)
 * 
 * @param pipeline_name 
 * @param node_name 
 * @param port 
 * @return true 
 * @return false 
 */
bool eagleeye_pipeline_debug_replace_at(const char* pipeline_name, const char* node_name, int port);

/**
 * @brief restore (debug pipeline)
 * 
 * @param pipeline_name 
 * @param node_name 
 * @param port 
 * @return true 
 * @return false 
 */
bool eagleeye_pipeline_debug_restore_at(const char* pipeline_name, const char* node_name, int port);


// 注册插件函数类型
typedef const char* (*REGISTER_PLUGIN_FUNC)();
// 初始化插件函数类型
typedef void (*INITIALIZE_PLUGIN_FUNC)();
/**
 * @brief add custom pipeline
 * 
 * @param pipeline_name 
 * @param func 
 * @return true 
 * @return false 
 */
bool eagleeye_custom_add_pipeline(REGISTER_PLUGIN_FUNC register_func, INITIALIZE_PLUGIN_FUNC init_func);

#define _LINK_MACRO(x) #x
#define EAGLEEYE_PIPELINE_REGISTER(pipeline, version, key) \
extern "C" { \
    const char* eagleeye_register_plugin_pipeline(){ \
        const char* pipeline_str = #pipeline; \
        const char* key_str = #key; \
        const char* version_str = #version; \
        bool result = eagleeye_register_pipeline(pipeline_str, version_str, key_str); \
        if(result){ \
            return pipeline_str; \
        } \
        else{ \
            return NULL; \
        } \
    } \
    void eagleeye_pipeline_initialize(); \
    bool eagleeye_##pipeline##_initialize(const char* config_folder){ \
        bool is_ok = eagleeye_custom_add_pipeline(eagleeye_register_plugin_pipeline, eagleeye_pipeline_initialize); \
        if(!is_ok){ \
            EAGLEEYE_LOGE("fail to register %s", #pipeline); \
            return false; \
        } \
        is_ok = eagleeye_pipeline_initialize(#pipeline, config_folder); \
        if(is_ok){ \
            EAGLEEYE_LOGD("success to initialize %s", #pipeline); \
        } \
        else{ \
            EAGLEEYE_LOGE("fail to initialize %s", #pipeline); \
        } \
        return is_ok; \
    } \
    bool eagleeye_##pipeline##_release(){ \
        return eagleeye_pipeline_release(#pipeline); \
    } \
    bool eagleeye_##pipeline##_run(){ \
        return eagleeye_pipeline_run(#pipeline); \
    } \
    bool eagleeye_##pipeline##_version(char* pipeline_version){ \
        return eagleeye_pipeline_version(#pipeline, pipeline_version); \
    } \
    bool eagleeye_##pipeline##_reset(){ \
        return eagleeye_pipeline_reset(#pipeline); \
    } \
    bool eagleeye_##pipeline##_set_param(const char* node_name, const char* param_name, const void* value){ \
        return eagleeye_pipeline_set_param(#pipeline, node_name, param_name, value); \
    } \
    bool eagleeye_##pipeline##_get_param(const char* node_name, const char* param_name, void* value){ \
        return eagleeye_pipeline_get_param(#pipeline, node_name, param_name, value); \
    } \
    bool eagleeye_##pipeline##_set_input(const char* node_name, void* data, const int* data_size, const int data_dims, const int data_type){ \
        return eagleeye_pipeline_set_input(#pipeline, node_name, data, data_size, data_dims, data_type); \
    } \
    bool eagleeye_##pipeline##_get_output(const char* node_name, void*& data, int* data_size, int& data_dims,int& data_type){ \
        return eagleeye_pipeline_get_output(#pipeline, node_name, data, data_size, data_dims, data_type); \
    }\
    bool eagleeye_##pipeline##_load_config(const char* config_file){ \
        return eagleeye_pipeline_load_config(#pipeline, config_file); \
    } \
    bool eagleeye_##pipeline##_save_config(const char* config_file){ \
        return eagleeye_pipeline_save_config(#pipeline, config_file); \
    } \
    bool eagleeye_##pipeline##_debug_replace_at(const char* node_name, int port){ \
        return eagleeye_pipeline_debug_replace_at(#pipeline, node_name, port); \
    } \
    bool eagleeye_##pipeline##_debug_restore_at(const char* node_name, int port){ \
        return eagleeye_pipeline_debug_restore_at(#pipeline, node_name, port); \
    } \
} 

#define EAGLEEYE_BEGIN_PIPELINE_INITIALIZE(pipeline) \
extern "C" { \
	void eagleeye_pipeline_initialize() { \
        AnyPipeline* pipeline = AnyPipeline::getInstance(#pipeline);

#define EAGLEEYE_END_PIPELINE_INITIALIZE }}

}
#endif