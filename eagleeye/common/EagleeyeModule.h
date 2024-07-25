#ifndef _EAGLEEYE_MODULE_H_
#define _EAGLEEYE_MODULE_H_
#include <string>
#include <vector>
#include <functional>
struct EagleeyeMeta{
	double fps;				// frame rate for video
	int nb_frames;			// frame number for video
	int frame;				// frame index for video
	bool is_end_frame;		// end frame flag for video
	bool is_start_frame; 	// start frame flag for video
	int rotation;			// rotation  (0,90,180,270)
	int rows;				// current rows 
	int cols;				// current cols
	int needed_rows;		// rows(largest)
	int needed_cols;		// cols(largest)
	int allocate_mode;		// 0（do nothing）;1（InPlace）;2（largest）;3（same size with input）;
	int64_t timestamp;	    // timestamp
};

struct EagleeyeConfig{
    std::string resource_folder;
};

namespace eagleeye{
/**
 * @brief pipeline initialize
 * @param pipeline_name
 * @param resource_folder
 */
bool eagleeye_pipeline_initialize(const char* pipeline_name, const char* resoure_folder=NULL);

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
 * @brief config system
 * 
 * @param config_content 
 * @return true 
 * @return false 
 */
bool eagleeye_config(EagleeyeConfig config_content);

/**
 * @brief release module
 * 
 * @return true 
 * @return false 
 */
bool eagleeye_release_module();

/**
 * @brief clear context
 * 
 * @return true 
 * @return false 
 */
bool eagleeye_clear_context();

/**
 * @brief register pipeline plugin
 * 
 * @param pipeline 
 * @return true 
 * @return false 
 */
bool eagleeye_register_pipeline(const char* pipeline, const char* version, const char* key);

/**
 * @brief reigster signal to background
 * @param pipeline pipeline name
 * @param node node/port
 */ 
bool eagleeye_register_signal(const char* pipeline, const char* node);

/**
 * @brief check whether node matching
 * @param pipeline pipeline name
 * @param node node/port
 * @param cache_node node/port register
 */ 
bool eagleeye_check_signal_matching(const char* pipeline, const char* node, const char* register_node);

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
 * @param pipeline_name
 * @param node_name 
 * @param data 
 * @param data_size 
 * @param data_dims 
 * @param data_rotation
 * @param data_type
 */
bool eagleeye_pipeline_set_input(const char* pipeline_name,
                                 const char* node_name, 
                                 void* data, 
                                 const size_t* data_size, 
                                 const int data_dims,
                                 const int data_rotation,
                                 const int data_type);

/**
 * @brief set pipeline input with meta
 * 
 * @param pipeline_name
 * @param node_name 
 * @param data 
 * @param meta
 */
bool eagleeye_pipeline_set_input(const char* pipeline_name,
                                 const char* node_name, 
                                 void* data, 
                                 EagleeyeMeta meta);

/**
 * @brief set pipeline input (path or node name)
 * @param pipeline_name 
 * @param node_name
 * @param register_node_name 
 */ 
bool eagleeye_pipeline_set_input(const char* pipeline_name,
                                 const char* node_name, 
                                 const char* register_node_name);

/**
 * @brief link pipelines
 */ 
bool eagleeye_pipeline_link(const char* target_pipeline_name,
                            const char* target_node_name,
                            const char* from_pipeline_name,
                            const char* from_node_name);

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
                                  size_t*& data_size, 
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
                                  size_t*& data_size, 
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
 * @brief pipeline run
 * 
 */
bool eagleeye_pipeline_run(const char* pipeline_name, const char* node_name=NULL, const char* ignore_prefix=NULL);

/**
 * @brief pipeline wait
 * 
 */
bool eagleeye_pipeline_wait(const char* pipeline_name, const char* ignore_prefix=NULL);

/**
 * @brief pipeline render
 */ 
bool eagleeye_pipeline_render(const char* pipeline_name, const char* ignore_prefix=NULL);

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

/**
 * @brief surface create
 */ 
bool eagleeye_on_surface_create();

/**
 * @brief surface size change
 */ 
bool eagleeye_on_surface_change(int width, int height, int rotate=0, bool mirror=false);

/**
 * @brief surface mouse event
 * @param mouse_x mouse pos x
 * @param mouse_y mouse pos y
 * @param mouse_flag 0: MOUSE_DOWN, 1: MOUSE_UP, 2: MOUSE_MOVE
 */ 
bool eagleeye_on_surface_mouse(int mouse_x, int mouse_y, int mouse_flag);


// 注册插件函数类型
typedef const char* (*REGISTER_PLUGIN_FUNC)();
// 初始化插件函数类型
typedef void* (*INITIALIZE_PLUGIN_FUNC)(void*);

/**
 * @brief pipeline server interface
 */

enum ServerStatus{
    SERVER_UNKOWN = -1,
    SERVER_SUCCESS,
    SERVER_TIMEOUT,
    SERVER_NOT_SUPPORT,
    SERVER_NOT_EXIST,
    SERVER_ABNORMAL,
    SERVER_ERROR
};
ServerStatus eagleeye_pipeline_server_init(std::string folder, std::vector<std::string> predefined_plugin_names=std::vector<std::string>());
ServerStatus eagleeye_pipeline_server_register(std::string server_name, INITIALIZE_PLUGIN_FUNC server_initialize_func);
ServerStatus eagleeye_pipeline_server_start(std::string server_config, std::string& server_key, std::function<void*(std::vector<void*>, void*)> render_config_func);
ServerStatus eagleeye_pipeline_server_call(std::string server_key, std::string request, std::string& reply, int timeout=3);
ServerStatus eagleeye_pipeline_server_render(std::string server_key);
ServerStatus eagleeye_pipeline_server_stop(std::string server_key);

/**
 *  @brief create compute node
 */
void* eagleeye_create_node(std::string node_cls_name);

/**
 *  @brief destroy compute node
 */
bool eagleeye_destroy_node(void* node_obj);

/**
 *  @brief bind sig to node/port
 */
void eagleeye_bind_node(void* node_obj, int node_port, void* sig);

/**
 * @brief add custom pipeline
 * 
 * @param pipeline_name 
 * @param register_func
 * @param  init_func
 * @return true 
 * @return false 
 */
bool eagleeye_custom_add_pipeline(const char* pipeline_name, REGISTER_PLUGIN_FUNC register_func, INITIALIZE_PLUGIN_FUNC init_func);

#define _LINK_MACRO(x) #x
#define EAGLEEYE_PIPELINE_REGISTER(pipeline, version, key) \
extern "C" { \
    const char* eagleeye_##pipeline##_register_plugin_pipeline(){ \
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
    void* eagleeye_##pipeline##_pipeline_initialize(void*); \
    bool eagleeye_##pipeline##_initialize(const char* config_folder){ \
        bool is_ok = eagleeye_custom_add_pipeline(#pipeline, eagleeye_##pipeline##_register_plugin_pipeline, eagleeye_##pipeline##_pipeline_initialize); \
        if(!is_ok){ \
            return false; \
        } \
        is_ok = eagleeye_pipeline_initialize(#pipeline, config_folder); \
        if(is_ok){ \
            EAGLEEYE_LOGD("Success to initialize %s.", #pipeline); \
        } \
        else{ \
            EAGLEEYE_LOGE("Fail to initialize %s.", #pipeline); \
        } \
        return is_ok; \
    } \
    bool eagleeye_##pipeline##_release(){ \
        return eagleeye_pipeline_release(#pipeline); \
    } \
    bool eagleeye_##pipeline##_run(const char* node_name=NULL, const char* ignore_prefix=NULL){ \
        return eagleeye_pipeline_run(#pipeline, node_name, ignore_prefix); \
    } \
    bool eagleeye_##pipeline##_wait(){ \
        return eagleeye_pipeline_wait(#pipeline); \
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
    bool eagleeye_##pipeline##_set_input(const char* node_name, void* data, const size_t* data_size, const int data_dims, const int data_rotation, const int data_type){ \
        return eagleeye_pipeline_set_input(#pipeline, node_name, data, data_size, data_dims, data_rotation, data_type); \
    } \
    bool eagleeye_##pipeline##_set_input2(const char* node_name, void* data, EagleeyeMeta meta){ \
        return eagleeye_pipeline_set_input(#pipeline, node_name, data, meta); \
    } \
    bool eagleeye_##pipeline##_get_output(const char* node_name, void*& data, size_t*& data_size, int& data_dims,int& data_type){ \
        return eagleeye_pipeline_get_output(#pipeline, node_name, data, data_size, data_dims, data_type); \
    } \
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
	void* eagleeye_##pipeline##_pipeline_initialize(void* extern_pipeline=NULL) { \
        AnyPipeline* pipeline = NULL; \
        if(extern_pipeline == NULL){ \
            pipeline = AnyPipeline::getInstance(#pipeline); \
        } \
        else{ \
            pipeline = (AnyPipeline*)extern_pipeline; \
        }

#define EAGLEEYE_END_PIPELINE_INITIALIZE(pipeline) \
    return pipeline; \
}} \
class pipeline##Pipeline:public AnyPipeline, DynamicPipelineCreator<pipeline##Pipeline>{ \
public: \
    pipeline##Pipeline(){ \
        eagleeye_##pipeline##_pipeline_initialize(this); \
        this->initialize(NULL, NULL, true); \
    } \
};

}
#endif