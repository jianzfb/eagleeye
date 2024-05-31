#include "eagleeye/common/EagleeyeModule.h"
#include "eagleeye/framework/pipeline/AnyPipeline.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeFile.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/runtime/gpu/opencl_runtime.h"
#include "eagleeye/common/EagleeyeFactory.h"
#include "eagleeye/common/EagleeyeRegisterCenter.h"
#include "eagleeye/common/EagleeyeCameraCenter.h"
#include "eagleeye/common/EagleeyeMessageCenter.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"
#include "eagleeye/processnode/AutoPipeline.h"
#ifdef EAGLEEYE_OPENGL
#include "eagleeye/common/EagleeyeShader.h"
#endif
#include <dlfcn.h>
#include <dirent.h>
#include <cstring>
#include "eagleeye/common/CJsonObject.hpp"

namespace eagleeye{
void _getAllPluginsFromDirectory(std::string path, std::vector<std::string> &files) {
        DIR *dp = NULL;
        struct dirent *dirp;
        if ((dp = opendir(path.c_str())) == NULL) {
            EAGLEEYE_LOGE("couldnt open plugin dir %s", path.c_str());
            return;
        }

        while ((dirp = readdir(dp)) != NULL) {
            // linux DT_DIR文件夹；DT_REG文件
            if(dirp->d_type & DT_DIR){
                // 文件夹
                std::string dir_name = dirp->d_name;
                // 检查是否存在同名.so
                std::string plugin_so_path;
                if(path.at(path.length() - 1) == '/'){
                    plugin_so_path = path + dir_name+"/lib"+dir_name+".so";
                }
                else{
                    plugin_so_path = path + "/" + dir_name+"/lib"+dir_name+".so";
                }
                bool is_exist = isfileexist(plugin_so_path.c_str());
                if(!is_exist){
                    continue;
                }
                EAGLEEYE_LOGD("plugin so path %s", plugin_so_path.c_str());

                // 检查是否存在同名.ini
                // std::string plugin_so_path = path + "/" + dir_name+"/"+dir_name+".ini";

                files.push_back(dir_name+"/lib"+dir_name+".so");
            }
        }
        if(dp != NULL){
            closedir(dp);
        }
        return ;
}

// registed plugins
std::map<std::string, std::pair<void*,INITIALIZE_PLUGIN_FUNC>> m_registed_plugins;
bool eagleeye_init_module(std::vector<std::string>& pipeline_names, const char* plugin_folder){
    EAGLEEYE_LOGD("Init module from %s.", plugin_folder);
    // 设置插件根目录
    AnyPipeline::setPluginRoot(plugin_folder);

    // 加载所有插件，发现注册模块
    EAGLEEYE_LOGD("Traverse to find all plugin in %s.",plugin_folder);
    std::vector<std::string> plugin_list;
    _getAllPluginsFromDirectory(plugin_folder, plugin_list);

    for(int index=0; index<plugin_list.size(); ++index){
        // 过滤已经发现的插件
        if(m_registed_plugins.find(plugin_list[index]) != m_registed_plugins.end()){
            EAGLEEYE_LOGD("Plugin %s has existed.", plugin_list[index].c_str());
            continue;
        }

        std::string plugin_path = std::string(plugin_folder) + "/" + plugin_list[index];
        // 加载so
        EAGLEEYE_LOGD("Load plugin %s from path %s.", plugin_list[index].c_str(), plugin_path.c_str());
        void* handle = dlopen(plugin_path.c_str(), RTLD_LAZY);
        if(!handle){        
            EAGLEEYE_LOGD("Dlopen error, message (%s).",dlerror());
            continue;
        }

        // 插件目录结构
        EAGLEEYE_LOGD("Plugin directory map.");
        std::string sperator="/";
        std::vector<std::string> kv = split(plugin_list[index], sperator);
        std::string plugin_path_parent = std::string(plugin_folder) + "/" + kv[0];
        // traverseFiles(plugin_path_parent.c_str());

        // 加载注册及初始化函数
        EAGLEEYE_LOGD("Get register plugin func.");
        REGISTER_PLUGIN_FUNC plugin_register_func = NULL; 
        INITIALIZE_PLUGIN_FUNC plugin_initialize_func = NULL;
        std::string register_func_name = "eagleeye_"+kv[0]+"_register_plugin_pipeline";
        plugin_register_func = (REGISTER_PLUGIN_FUNC)dlsym(handle,register_func_name.c_str());
        if(!plugin_register_func){
            EAGLEEYE_LOGD("Dlsym error, message (%s).",dlerror());
            dlclose(handle);
            continue;
        }        

        EAGLEEYE_LOGD("Finish get register plugin func.");
        const char* plugin_name = plugin_register_func();
        if(plugin_name == NULL){
            EAGLEEYE_LOGD("Plugin %s fail to register.");
            continue;
        }

        EAGLEEYE_LOGD("Get initialize plugin func.");
        std::string init_func_name = "eagleeye_"+kv[0]+"_pipeline_initialize";
        plugin_initialize_func = (INITIALIZE_PLUGIN_FUNC)dlsym(handle, init_func_name.c_str());
        if(!plugin_initialize_func){
            EAGLEEYE_LOGD("Dlsym error, message (%s)",dlerror());
            dlclose(handle);
            continue;
        }        

        EAGLEEYE_LOGD("Finish initialize plugin func.");
        AnyPipeline* pipeline = AnyPipeline::getInstance(plugin_name);
        if(pipeline == NULL){
            EAGLEEYE_LOGD("Couldnt get pipeline instance.");
        }
        pipeline->setInitFunc(plugin_initialize_func);

        // add to 
        m_registed_plugins[std::string(plugin_name)] = std::make_pair(handle,plugin_initialize_func);
    }

    // 获得所有注册的pipeline
    AnyPipeline::getRegistedPipelines(pipeline_names);
    if(pipeline_names.size() == 0){
        EAGLEEYE_LOGD("Found 0 pipeline plugin.");
        return false;
    }
    
    EAGLEEYE_LOGD("Found %d pipeline plugins.", pipeline_names.size());

    // 初始化上下文
    OpenCLRuntime::getOpenCLEnv();
    return true;
}

bool eagleeye_release_module(){
    EAGLEEYE_LOGD("Enter release plugin handler.");
    std::map<std::string, std::pair<void*,INITIALIZE_PLUGIN_FUNC>>::iterator iter,iend(m_registed_plugins.end());

    for(iter=m_registed_plugins.begin(); iter != iend; ++iter){
        EAGLEEYE_LOGD("Release so %s.", iter->first.c_str());
        if(iter->second.first != NULL){
             dlclose(iter->second.first);
        }
    }
    
    m_registed_plugins.clear();
    EAGLEEYE_LOGD("Finish release plugin handler.");
    return true;
}

bool eagleeye_custom_add_pipeline(const char* pipeline_name, REGISTER_PLUGIN_FUNC register_func, INITIALIZE_PLUGIN_FUNC init_func){
    if(m_registed_plugins.find(std::string(pipeline_name)) != m_registed_plugins.end()){
        return true;
    }
    
    const char* register_pipeline_name = register_func();
    if(register_pipeline_name == NULL){
        EAGLEEYE_LOGE("Resiger fail.");
        return false;
    }

    m_registed_plugins[std::string(register_pipeline_name)] = std::pair<void*,INITIALIZE_PLUGIN_FUNC>(NULL,init_func);
    return true;
}

bool eagleeye_pipeline_initialize(const char* pipeline_name, const char* resource_folder){
    if(!AnyPipeline::isExist(pipeline_name)){
        // 1.step 从文件中加载
        std::string pipeline_file = AnyPipeline::getPluginRoot()+"/"+pipeline_name+"/"+pipeline_name+".json";
        if(isfileexist(pipeline_file.c_str())){
            AnyPipeline* pipeline = eagleeye_build_pipeline_from_json(pipeline_file.c_str(), resource_folder);            
            if(pipeline != NULL){
                return true;
            }
        }

        EAGLEEYE_LOGD("Pipeline %s dont exist", pipeline_name);
        return false;
    }

    // 2.step 从插件库中加载
    AnyPipeline::getInstance(pipeline_name)->setInitFunc(m_registed_plugins[std::string(pipeline_name)].second);
    AnyPipeline::getInstance(pipeline_name)->setPipelineName(pipeline_name);
    AnyPipeline::getInstance(pipeline_name)->initialize(resource_folder);
    return true;
}

bool eagleeye_config(EagleeyeConfig config_content){
    if(config_content.resource_folder != ""){
        // 设置全局资源路径
        AnyNode::setResourceFolder(config_content.resource_folder);
    }
}

bool eagleeye_pipeline_load_config(const char* pipeline_name, const char* config_file){
    EAGLEEYE_LOGD("Load pipeline %s from configure file %s.", pipeline_name, config_file);
    AnyPipeline::getInstance(pipeline_name)->loadConfigure(config_file);
    return true;
}

bool eagleeye_pipeline_save_config(const char* pipeline_name, const char* config_file){
    EAGLEEYE_LOGD("Save pipeline %s to configure file %s.", pipeline_name, config_file);
    AnyPipeline::getInstance(pipeline_name)->saveConfigure(config_file);
    return true;
}

bool eagleeye_pipeline_release(const char* pipeline_name){
    EAGLEEYE_LOGD("Enter eagleeye_pipeline_release %s.", pipeline_name);    
    if(m_registed_plugins.find(std::string(pipeline_name)) == m_registed_plugins.end()){
        if(AnyPipeline::isExist(pipeline_name)){
            EAGLEEYE_LOGD("Finish eagleeye_pipeline_release %s.", pipeline_name);
            AnyPipeline::releasePipeline(pipeline_name);
            return true;
        }

        EAGLEEYE_LOGD("Pipeline %s dont existed.", pipeline_name);
        return false;
    }

    // 注销管线
    AnyPipeline::releasePipeline(pipeline_name);

    // 关闭动态库
    if(m_registed_plugins[std::string(pipeline_name)].first != NULL){
        // 关闭动态库
        dlclose(m_registed_plugins[std::string(pipeline_name)].first);      
    }

    // 删除注册记录
    m_registed_plugins.erase(m_registed_plugins.find(std::string(pipeline_name)));
    EAGLEEYE_LOGD("Finish eagleeye_pipeline_release %s.", pipeline_name);
    return true;
}

bool eagleeye_clear_context(){
#ifdef EAGLEEYE_OPENGL    
    // 清空opengl上下文
    ShaderManager::getInstance()->clear();
#endif
    // 清空opencl上下文
    return true;
}

bool eagleeye_pipeline_version(const char* pipeline_name, char* pipeline_version){
    const char* version = AnyPipeline::getInstance(pipeline_name)->getPipelineVersion();
    memcpy(pipeline_version, version, sizeof(char)*strlen(version) + 1);
    return true;
}

bool eagleeye_register_pipeline(const char* pipeline, const char* version, const char* key){
    EAGLEEYE_LOGD("Register %s pipeline.", pipeline);
    return AnyPipeline::registerPipeline(pipeline, version, key);
}

bool eagleeye_register_signal(const char* pipeline, const char* node){
    EAGLEEYE_LOGD("Register %s/%s signal.", pipeline, node);
    return AnyPipeline::registerSignal(pipeline, node);
}

bool eagleeye_check_signal_matching(const char* pipeline, const char* node, const char* register_node){
    return AnyPipeline::checkSignalMatching(pipeline, node, register_node);
}

bool eagleeye_pipeline_get_monitor_config(const char* pipeline_name,
                                          std::vector<std::string>& monitor_names,
                                          std::vector<int>& monitor_types,
                                          std::vector<std::string>& monitor_range){
    // get pipeline monitors
    AnyPipeline::getInstance(pipeline_name)->getPipelineMonitors(monitor_names, monitor_types, monitor_range);
    return true;
}

bool eagleeye_pipeline_get_input_config(const char* pipeline_name,
                                        std::vector<std::string>& input_nodes,
                                        std::vector<std::string>& input_types,
                                        std::vector<std::string>& input_category,
                                        std::vector<std::string>& input_sources){
    // get pipeline input   
    AnyPipeline::getInstance(pipeline_name)->getPipelineInputs(input_nodes, input_types, input_category, input_sources);
    return true;
}

bool eagleeye_pipeline_get_output_config(const char* pipeline_name,
                                         std::vector<std::string>& output_nodes,
                                         std::vector<std::string>& output_types,
                                         std::vector<std::string>& output_category,
                                         std::vector<std::string>& output_targets){
    // get pipeline output
    AnyPipeline::getInstance(pipeline_name)->getPipelineOutputs(output_nodes, output_types, output_category, output_targets);
    return true;
}

bool eagleeye_pipeline_set_param(const char* pipeline_name,
                                 const char* node_name, 
                                 const char* param_name, 
                                 const void* value){
    AnyPipeline::getInstance(pipeline_name)->setParameter(node_name, param_name, value);
    return true;
}

bool eagleeye_pipeline_get_param(const char* pipeline_name,
                                 const char* node_name, 
                                 const char* param_name, 
                                 void* value){
    AnyPipeline::getInstance(pipeline_name)->getParameter(node_name, param_name, value);
    return true;
}

bool eagleeye_pipeline_set_input(const char* pipeline_name,
                                 const char* node_name, 
                                 void* data, 
                                 const size_t* data_size, 
                                 const int data_dims,
                                 const int data_rotation,
                                 const int data_type){
    EAGLEEYE_LOGD("Set pipeline %s node %s input.", pipeline_name, node_name);
    AnyPipeline::getInstance(pipeline_name)->setInput(node_name, data, data_size, data_dims, data_rotation, data_type);
    return true;
}

bool eagleeye_pipeline_set_input(const char* pipeline_name,
                                 const char* node_name, 
                                 void* data, 
                                 EagleeyeMeta meta){
    EAGLEEYE_LOGD("Set pipeline %s node %s input.", pipeline_name, node_name);
    MetaData inner_meta;
    inner_meta.fps = meta.fps;
    inner_meta.nb_frames= meta.nb_frames;
    inner_meta.frame = meta.frame;
    inner_meta.is_end_frame = meta.is_end_frame;
    inner_meta.is_start_frame = meta.is_start_frame;
    inner_meta.rotation = meta.rotation;
    inner_meta.rows = meta.rows;
    inner_meta.cols = meta.cols;
    inner_meta.needed_rows = meta.needed_rows;
    inner_meta.needed_cols = meta.needed_cols;
    inner_meta.allocate_mode = meta.allocate_mode;
    inner_meta.timestamp = meta.timestamp;
    AnyPipeline::getInstance(pipeline_name)->setInput(node_name, data, inner_meta);
    return true;
}

bool eagleeye_pipeline_set_input(const char* pipeline_name,
                                 const char* node_name, 
                                 const char* register_node_name){
    EAGLEEYE_LOGD("Set pipeline %s node %s input.", pipeline_name, node_name);
    AnyPipeline::getInstance(pipeline_name)->setInput(node_name, register_node_name);
    return true;
}

bool eagleeye_pipeline_link(const char* target_pipeline_name,
                            const char* target_node_name,
                            const char* from_pipeline_name,
                            const char* from_node_name){
    EAGLEEYE_LOGD("Link pipeline %s node %s to target pipeline %s node %s.",
                     from_pipeline_name, 
                     from_node_name, 
                     target_pipeline_name, 
                     target_node_name);
    AnyPipeline::getInstance(target_pipeline_name)->setInput(target_node_name, from_pipeline_name, from_node_name);
    return true;
}

bool eagleeye_pipeline_get_node_output(const char* pipeline_name,
                                  const char* node_name, 
                                  void*& data, 
                                  size_t*& data_size, 
                                  int& data_dims,
                                  int& data_type){
    EAGLEEYE_LOGD("%s pipeline get data from node %s", pipeline_name, node_name);
    AnyPipeline::getInstance(pipeline_name)->getNodeOutput(node_name, data, data_size, data_dims, data_type);
    return true;
}

bool eagleeye_pipeline_get_output(const char* pipeline_name,
                                  const char* node_name, 
                                  void*& data, 
                                  size_t*& data_size, 
                                  int& data_dims,
                                  int& data_type){
    EAGLEEYE_LOGD("%s pipeline get data from node %s", pipeline_name, node_name);
    AnyPipeline::getInstance(pipeline_name)->getOutput(node_name, data, data_size, data_dims, data_type);
    return true;
}

bool eagleeye_pipeline_run(const char* pipeline_name, const char* node_name, const char* ignore_prefix){
    std::string s_pipeline_name = pipeline_name;
    if(s_pipeline_name.find("/") != std::string::npos){
        std::vector<std::string> kterms = split(s_pipeline_name, "/");
        std::string pn = kterms[0];
        std::string nn = kterms[1];
        if(AnyPipeline::getInstance(pn.c_str()) == NULL){
            return false;
        }

        AnyPipeline::getInstance(pn.c_str())->refresh();
        bool r = AnyPipeline::getInstance(pn.c_str())->start(nn.c_str());
        return r;
    }

    if(AnyPipeline::getInstance(pipeline_name) == NULL){
        return false;
    }
    bool result = AnyPipeline::getInstance(pipeline_name)->start(node_name, ignore_prefix);
    return result;
}

bool eagleeye_pipeline_wait(const char* pipeline_name, const char* ignore_prefix){
    std::string s_pipeline_name = pipeline_name;
    if(s_pipeline_name.find("/") != std::string::npos){
        std::vector<std::string> kterms = split(s_pipeline_name, "/");
        std::string pn = kterms[0];
        std::string nn = kterms[1];
        
        if(AnyPipeline::getInstance(pn.c_str()) == NULL){
            return false;
        }
        AnyPipeline::getInstance(pn.c_str())->wait(nn.c_str());
        return true;
    }

    if(AnyPipeline::getInstance(pipeline_name) == NULL){
        return false;
    }
    AnyPipeline::getInstance(pipeline_name)->wait(NULL, ignore_prefix);
    return true;
}

bool eagleeye_pipeline_render(const char* pipeline_name, const char* ignore_prefix){
    if(AnyPipeline::getInstance(pipeline_name) == NULL){
        return false;
    }

    // 强制渲染节点更新
    AnyPipeline::getInstance(pipeline_name)->refresh();

    // 运行
    bool result = eagleeye_pipeline_run(pipeline_name, ignore_prefix);
    return result;
} 

bool eagleeye_pipeline_reset(const char* pipeline_name){
    if(AnyPipeline::getInstance(pipeline_name) == NULL){
        return false;
    }

    AnyPipeline::getInstance(pipeline_name)->reset();
    return true;
}

bool eagleeye_pipeline_debug_replace_at(const char* pipeline_name, const char* node_name, int port){
    EAGLEEYE_LOGD("replace node %s at port %d in pipeline %s", node_name, port, pipeline_name);
    AnyPipeline::getInstance(pipeline_name)->replaceAt(node_name, port);
    return true;
}

bool eagleeye_pipeline_debug_restore_at(const char* pipeline_name, const char* node_name, int port){
    EAGLEEYE_LOGD("restore node %s at port %d in pipeline %s", node_name, port, pipeline_name);
    AnyPipeline::getInstance(pipeline_name)->restoreAt(node_name, port);
    return true;
}

bool eagleeye_on_surface_create(){
    EAGLEEYE_LOGD("Surface create.");
    AnyPipeline::onRenderSurfaceCreate();
    return true;
}

bool eagleeye_on_surface_change(int width, int height, int rotate, bool mirror){
    EAGLEEYE_LOGD("Surface width %d height %d.", width, height);
    AnyPipeline::onRenderSurfaceChange(width, height, rotate, mirror);
    return true;
}

bool eagleeye_on_surface_mouse(int mouse_x, int mouse_y, int mouse_flag){
    AnyPipeline::onRenderSurfaceMouse(mouse_x, mouse_y, mouse_flag);
    return true;
}

std::map<std::string, INITIALIZE_PLUGIN_FUNC> pipeline_init_map;
ServerStatus eagleeye_pipeline_server_init(std::string folder){
    EAGLEEYE_LOGD("Traverse to find all plugin in %s.", folder.c_str());
    std::vector<std::string> plugin_list;
    _getAllPluginsFromDirectory(folder, plugin_list);

    for(int index=0; index<plugin_list.size(); ++index){
        std::string plugin_path = std::string(folder) + "/" + plugin_list[index];
        // 加载so
        EAGLEEYE_LOGD("Load plugin %s from path %s.", plugin_list[index].c_str(), plugin_path.c_str());
        void* handle = dlopen(plugin_path.c_str(), RTLD_LAZY);
        if(!handle){        
            EAGLEEYE_LOGD("Dlopen error, message (%s).",dlerror());
            continue;
        }

        // 插件目录结构
        std::string sperator="/";
        std::vector<std::string> kv = split(plugin_list[index], sperator);
        std::string plugin_path_parent = std::string(folder) + "/" + kv[0];

        // 加载注册及初始化函数
        REGISTER_PLUGIN_FUNC plugin_register_func = NULL; 
        INITIALIZE_PLUGIN_FUNC plugin_initialize_func = NULL;

        EAGLEEYE_LOGD("Get initialize plugin func.");
        std::string init_func_name = "eagleeye_"+kv[0]+"_pipeline_initialize";
        plugin_initialize_func = (INITIALIZE_PLUGIN_FUNC)dlsym(handle, init_func_name.c_str());
        if(!plugin_initialize_func){
            EAGLEEYE_LOGD("Dlsym error, message (%s)",dlerror());
            dlclose(handle);
            continue;
        }

        pipeline_init_map[kv[0]] = plugin_initialize_func;
    }

    return SERVER_SUCCESS;
}

ServerStatus eagleeye_pipeline_server_register(std::string server_name, INITIALIZE_PLUGIN_FUNC server_initialize_func){
    if(server_initialize_func == NULL){
        return SERVER_ERROR;
    }

    pipeline_init_map[server_name] = server_initialize_func;
    EAGLEEYE_LOGD("Success register pipeline %s", server_name.c_str());
    return SERVER_SUCCESS;
}


ServerStatus eagleeye_pipeline_server_start(std::string server_config, std::string& server_key, std::function<void*(std::vector<void*>, void*)> render_config_func){
    // 1.step 解析request
    // {
    //      "pipeline_name": "", 
    //      "server_params": [{"node": "node_name", "name": "param_name", "value": "param_value", "type": "string"/"float"/"double"/"int"/"bool"}], 
    //      "server_timestamp": "",
    //      "server_id": "",
    //      "data_source": [{"type": "camera", "address": "", "format": "RGB/BGR", "mode": "NETWORK/USB/ANDROID_NATIVE", "flag": "front"}, {"type": "video", "address": "", "format": "RGB/BGR"},...]
    // }
    neb::CJsonObject config_obj(server_config);
    std::string pipeline_name;
    config_obj.Get("pipeline_name", pipeline_name);
    std::string server_id;
    config_obj.Get("server_id", server_id);
    std::string server_timestamp;
    config_obj.Get("server_timestamp", server_timestamp);
    neb::CJsonObject server_params;
    config_obj.Get("server_params", server_params);
    neb::CJsonObject data_source;
    config_obj.Get("data_source", data_source);
    EAGLEEYE_LOGD("Receive server_id %s, server_timestamp %s, pipeline_name %s", server_id.c_str(), server_timestamp.c_str(), pipeline_name.c_str());

    // 2.step 清理存在的服务
    if(server_id != "" && RegisterCenter::getInstance()->hasObjWithPrefix(server_id)){
        // 清理现存的所有算法管线
        EAGLEEYE_LOGD("Clear exist %s related pipelins", server_id.c_str());
        RegisterCenter::getInstance()->destroyObjWithPrefix(server_id);
    }

    // 3.step 根据模式构建管线执行
    if(!data_source.IsEmpty()){
        // 回调构建 (AutoNode + AutoPipeline)
        // AutoNode 封装数据读取
        std::vector<std::string> source_list;
        for(int source_i=0; source_i<data_source.GetArraySize(); ++source_i){
            neb::CJsonObject source_cfg;
            data_source.Get(source_i, source_cfg);
            std::string source_type;
            source_cfg.Get("type", source_type);
            std::string source_address;
            source_cfg.Get("address", source_address);
            std::string source_format;
            source_cfg.Get("format", source_format);
            EAGLEEYE_LOGD("Data source %d, type=%s, address=%s", source_i, source_type.c_str(), source_address.c_str());

            if(source_type == "camera"){
                std::string source_mode;
                source_cfg.Get("mode", source_mode);
                if(source_mode == "NETWORK"){
                    if(source_format == "BGR"){
                        EAGLEEYE_LOGD("Create RTSP source (BGR).");
                        CameraCenter::getInstance()->addCamera(source_address, 1, CAMERA_NETWORK);
                    }
                    else{
                        EAGLEEYE_LOGD("Create RTSP source (RGB).");
                        CameraCenter::getInstance()->addCamera(source_address, 0, CAMERA_NETWORK);
                    }
                    source_list.push_back(source_address);
                }
                else if(source_mode == "USB"){
                    EAGLEEYE_LOGE("Not support USB camera now.");
                    return SERVER_NOT_SUPPORT;
                }
                else if(source_mode == "ANDROID_NATIVE"){
                    if(source_format == "BGR"){
                        EAGLEEYE_LOGD("Create android native camera source (BGR).");
                        CameraCenter::getInstance()->addCamera(source_address, 1, CAMERA_ANDROID_NATIVE);
                    }
                    else{
                        EAGLEEYE_LOGD("Create android native camera source (RGB).");
                        CameraCenter::getInstance()->addCamera(source_address, 0, CAMERA_ANDROID_NATIVE);
                    }
                    source_list.push_back(source_address);
                }
            }
            else if(source_type == "video"){
                if(source_format == "BGR"){
                    EAGLEEYE_LOGD("Create video source (BGR).");                        
                    CameraCenter::getInstance()->addCamera(source_address, 1, CAMERA_VIDEO);
                }
                else{
                    EAGLEEYE_LOGD("Create video source (RGB).");                        
                    CameraCenter::getInstance()->addCamera(source_address, 0, CAMERA_VIDEO);
                }
                source_list.push_back(source_address);
            }
        }

        // AutoPipeline 封装管线处理
        if(pipeline_init_map.find(pipeline_name) == pipeline_init_map.end()){
            EAGLEEYE_LOGE("pipeline %s not register.", pipeline_name.c_str());
            return SERVER_NOT_EXIST;
        }

        std::string key = server_id + "/" + server_timestamp;
        EAGLEEYE_LOGD("Construct pipeline.");
        AnyNode* auto_pipeline_node = new AutoPipeline(
            [&](){
                AnyPipeline* pipeline = new AnyPipeline();
                pipeline->setPipelineName(key.c_str());

                // 初始化管线
                pipeline_init_map[pipeline_name](pipeline);
                const char* config_folder = NULL;
                pipeline->initialize(config_folder, nullptr, true);

                // 配置管线参数
                if(!server_params.IsEmpty()){
                    for(int i=0; i<server_params.GetArraySize(); ++i){
                        neb::CJsonObject node_param_info = server_params[i];

                        std::string node_name;
                        node_param_info.Get("node", node_name);
                        std::string param_name;
                        node_param_info.Get("name", param_name);
                        std::string param_type;
                        node_param_info.Get("type", param_type);
                        // "string"/"float"/"double"/"int"/"bool"
                        if(param_type == "string"){
                            std::string param_value;
                            bool is_ok = node_param_info.Get('value', param_value);
                            if(is_ok){
                                pipeline->setParameter(node_name.c_str(), param_name.c_str(), &param_value);
                            }
                        }
                        else if(param_type == "float"){
                            float param_value = 0.0f;
                            bool is_ok = node_param_info.Get('value', param_value);
                            if(is_ok){
                                pipeline->setParameter(node_name.c_str(), param_name.c_str(), &param_value);
                            }
                        }
                        else if(param_type == "double"){
                            double param_value = 0.0;
                            bool is_ok = node_param_info.Get('value', param_value);
                            if(is_ok){
                                pipeline->setParameter(node_name.c_str(), param_name.c_str(), &param_value);
                            }
                        }
                        else if(param_type == "int"){
                            int param_value = 0;
                            bool is_ok = node_param_info.Get('value', param_value);
                            if(is_ok){
                                pipeline->setParameter(node_name.c_str(), param_name.c_str(), &param_value);
                            }
                        }
                        else if(param_type == "bool"){
                            bool param_value = false;
                            bool is_ok = node_param_info.Get('value', param_value);
                            if(is_ok){
                                pipeline->setParameter(node_name.c_str(), param_name.c_str(), &param_value);
                            }
                        }
                    }
                }

                // 自定义回调
                // 需要在管线内部完成构建

                return pipeline;
            },
            std::vector<std::pair<std::string, int>>({}),
            1
        );

        // 关联 AutoNode -> AutoPipeline
        auto_pipeline_node->setNumberOfInputSignals(source_list.size());
        std::vector<void*> source_sigs;
        bool is_ok = true;
        for(int source_i=0; source_i<source_list.size(); ++source_i){
            AnyNode* source_node = CameraCenter::getInstance()->getCamera(source_list[source_i]);
            if(source_node == NULL){
                is_ok = false;
                break;
            }
            AnySignal* source_sig = source_node->getOutputPort(0);
            auto_pipeline_node->setInputPort(source_sig, source_i);
            source_sigs.push_back(source_sig);
        }
        if(!is_ok){
            // 数据源存在问题，退出，清理
            EAGLEEYE_LOGE("Fail bind data source to pipeline.");
            delete auto_pipeline_node;
            return SERVER_ABNORMAL;
        }

        // 关联 RenderNode
        if(render_config_func != nullptr){
            std::string render_key = server_id + "/" + server_timestamp + "/render";
            EAGLEEYE_LOGD("Construct render pipeline with config_func, and register as %s.", render_key.c_str());
            AnyNode* render_node = (AnyNode*)(render_config_func(source_sigs, auto_pipeline_node));
            RegisterCenter::getInstance()->registerObj(
                render_key,
                render_node,
                [](std::string k, void* obj){
                    // 删除渲染
                    EAGLEEYE_LOGD("Delete render node %s", k.c_str());
                    AnyNode* node = (AnyNode*)obj;
                    node->exit();

                    for(int sig_i=0; sig_i<node->getNumberOfInputSignals(); ++sig_i){
                        node->clearInputPort(sig_i);
                    }
                    delete node;
                }
            );
        }

        // 启动 AutoPipeline
        auto_pipeline_node->init();

        // 注册管线到管线管理中心
        EAGLEEYE_LOGD("Register pipeline as %s.", key.c_str());
        bool is_success_register = RegisterCenter::getInstance()->registerObj(
            key, 
            auto_pipeline_node, 
            [source_list](std::string pipeline_key, void* pipeline_obj){
                // 1.step 删除管线
                EAGLEEYE_LOGD("Delete pipeline %s", pipeline_key.c_str());
                AnyNode* node = (AnyNode*)pipeline_obj;
                node->exit();

                for(int sig_i=0; sig_i<node->getNumberOfInputSignals(); ++sig_i){
                    node->clearInputPort(sig_i);
                }
                delete node;

                // 2.step 清空相机占用
                EAGLEEYE_LOGD("Clear pipeline %s occupy camera", pipeline_key.c_str());
                for(int camera_i=0; camera_i<source_list.size(); camera_i+=1){
                    EAGLEEYE_LOGD("Clear camera %d -> %s", camera_i, source_list[camera_i].c_str());
                    CameraCenter::getInstance()->removeCamera(source_list[camera_i]);
                }

                // 3.step 清空消息队列
                EAGLEEYE_LOGD("Clear message %s", pipeline_key.c_str());
                MessageCenter::getInstance()->clear(pipeline_key);
            },
            "mode/callback"
        );

        if(!is_success_register){
            EAGLEEYE_LOGE("Register pipeline manager fail.");
            return SERVER_ABNORMAL;            
        }

        // 注册到消息中心
        is_success_register = MessageCenter::getInstance()->create(key);
        if(!is_success_register){
            EAGLEEYE_LOGE("Register to message center fail.");
            return SERVER_ABNORMAL;
        }
        server_key = key;
    }
    else{
        // 直接构建
        if(pipeline_init_map.find(pipeline_name) == pipeline_init_map.end()){
            EAGLEEYE_LOGE("pipeline %s not register.", pipeline_name.c_str());
            return SERVER_NOT_EXIST;
        }  

        std::string key = server_id + "/" + server_timestamp;
        AnyPipeline* pipeline = new AnyPipeline();
        pipeline->setPipelineName(key.c_str());

        // 初始化管线
        pipeline_init_map[pipeline_name](pipeline);
        const char* config_folder = NULL;
        pipeline->initialize(config_folder, nullptr, true);

        // 配置管线参数
        if(!server_params.IsEmpty()){
            for(int i=0; i<server_params.GetArraySize(); ++i){
                neb::CJsonObject node_param_info = server_params[i];

                std::string node_name;
                node_param_info.Get("node", node_name);
                std::string param_name;
                node_param_info.Get("name", param_name);
                std::string param_type;
                node_param_info.Get("type", param_type);
                // "string"/"float"/"double"/"int"/"bool"
                if(param_type == "string"){
                    std::string param_value;
                    bool is_ok = node_param_info.Get('value', param_value);
                    if(is_ok){
                        pipeline->setParameter(node_name.c_str(), param_name.c_str(), &param_value);
                    }
                }
                else if(param_type == "float"){
                    float param_value = 0.0f;
                    bool is_ok = node_param_info.Get('value', param_value);
                    if(is_ok){
                        pipeline->setParameter(node_name.c_str(), param_name.c_str(), &param_value);
                    }
                }
                else if(param_type == "double"){
                    double param_value = 0.0;
                    bool is_ok = node_param_info.Get('value', param_value);
                    if(is_ok){
                        pipeline->setParameter(node_name.c_str(), param_name.c_str(), &param_value);
                    }
                }
                else if(param_type == "int"){
                    int param_value = 0;
                    bool is_ok = node_param_info.Get('value', param_value);
                    if(is_ok){
                        pipeline->setParameter(node_name.c_str(), param_name.c_str(), &param_value);
                    }
                }
                else if(param_type == "bool"){
                    bool param_value = false;
                    bool is_ok = node_param_info.Get('value', param_value);
                    if(is_ok){
                        pipeline->setParameter(node_name.c_str(), param_name.c_str(), &param_value);
                    }
                }
            }
        }

        // 注册到中心
        EAGLEEYE_LOGD("Register pipeline as %s.", key.c_str());
        bool is_success_register = RegisterCenter::getInstance()->registerObj(
            key,
            pipeline,
            [](std::string pipeline_key, void* pipeline_obj){
                // 1.step 删除管线
                EAGLEEYE_LOGD("Delete pipeline %s", pipeline_key.c_str());
                AnyPipeline* waiting_del_pipeline = (AnyPipeline*)pipeline_obj;
                delete waiting_del_pipeline;
            }
        );
        if(!is_success_register){
            EAGLEEYE_LOGE("Register pipeline fail.");
            return SERVER_ABNORMAL;
        }

        server_key = key;
    }

    return SERVER_SUCCESS;
}

ServerStatus eagleeye_pipeline_server_call(std::string server_key, std::string request, std::string& reply, int timeout){
    void* pipeline_obj = RegisterCenter::getInstance()->getObj(server_key);
    if(pipeline_obj == NULL){
        return SERVER_NOT_EXIST;
    }
    // 管线模式
    std::string mode = RegisterCenter::getInstance()->getInfo(server_key);

    // 执行
    if(mode == "mode/callback"){
        // 回调模式（接收reply, 返回reply）
        std::shared_ptr<Message> message = MessageCenter::getInstance()->get(server_key, timeout);
        if(message.get() == nullptr){
            return SERVER_TIMEOUT;
        }

        reply = message->serialize();
    }
    else{
        // 直建模式（处理request，返回reply）
        // {
        //      "data": [
        //          {"type": "image", "content": "", "width": 0, "height": 0, "channel": 0}, 
        //          {"type": "string", "content": ""}, 
        //          {"type": "matrix/float", "content": "", "width":0, "height":0},
        //          {"type": "matrix/int32", "content": "", "width":0, "height":0}
        //       ]
        // }

        AnyPipeline* pipeline = (AnyPipeline*)pipeline_obj;
        // 1.step 解析request
        neb::CJsonObject request_obj(request);   
        neb::CJsonObject data_info;
        request_obj.Get("data", data_info);
        for(int data_i=0; data_i<data_info.GetArraySize(); ++data_i){
            neb::CJsonObject data_cfg;
            data_info.Get(data_i, data_cfg);
            std::string data_type = "";
            data_cfg.Get("type", data_type);

            std::string data_i_placeholder = "placeholder_"+std::to_string(data_i);
            if(data_type == "image"){
                int width = 0;
                int height = 0;
                int channel = 0;
                data_cfg.Get("width", width);
                data_cfg.Get("height", height);
                data_cfg.Get("channel", channel);
                std::vector<size_t> data_size = {(size_t)height, (size_t)width, (size_t)channel};

                int64 data_content;   // 内存地址
                data_cfg.Get("content", data_content);
                void* data_content_ptr = (void*)(data_content);
                if(channel == 3){
                    pipeline->setInput(data_i_placeholder.c_str(), data_content_ptr, data_size.data(), data_size.size(), 0, 8);
                }
                else if(channel == 4){
                    pipeline->setInput(data_i_placeholder.c_str(), data_content_ptr, data_size.data(), data_size.size(), 0, 9);
                }
            }
            else if(data_type == "string"){
                std::string data;
                data_cfg.Get("content", data);
                void* data_content_ptr = (void*)(&data);
                std::vector<size_t> data_size = {(size_t)1, (size_t)data.size()};
                pipeline->setInput(data_i_placeholder.c_str(), data_content_ptr, data_size.data(), data_size.size(), 0, -1);
            }
            else if(data_type == "matrix/float"){
                int width = 0;
                int height = 0;
                data_cfg.Get("width", width);
                data_cfg.Get("height", height);
                std::vector<size_t> data_size = {(size_t)height, (size_t)width};

                int64 data_content;   // 内存地址
                data_cfg.Get("content", data_content);
                void* data_content_ptr = (void*)(data_content);

                pipeline->setInput(data_i_placeholder.c_str(), data_content_ptr, data_size.data(), data_size.size(), 0, 6);
            }
            else if(data_type == "matrix/int32"){
                int width = 0;
                int height = 0;
                data_cfg.Get("width", width);
                data_cfg.Get("height", height);
                std::vector<size_t> data_size = {(size_t)height, (size_t)width};

                int64 data_content;   // 内存地址
                data_cfg.Get("content", data_content);
                void* data_content_ptr = (void*)(data_content);

                pipeline->setInput(data_i_placeholder.c_str(), data_content_ptr, data_size.data(), data_size.size(), 0, 4);
            }
        }

        // 2.step 运行
        pipeline->start();

        // 3.step 获得结果，并序列化
        std::vector<std::string> output_nodes;
        std::vector<std::string> output_types;
        std::vector<std::string> output_categorys;
        std::vector<std::string> output_targets;
        pipeline->getPipelineOutputs(output_nodes, output_types, output_categorys, output_targets);
        for(int signal_i=0; signal_i<output_nodes.size(); ++signal_i){
            // SIGNAL_CATEGORY_STRING - 2
            if(output_categorys[signal_i] == "2"){
                void* temp_content;
                size_t* temp_size;
                int temp_dims;
                int temp_type;
                pipeline->getOutput(output_nodes[signal_i].c_str(), temp_content, temp_size, temp_dims, temp_type);

                reply = *((std::string*)temp_content);
                break;
            }
        }
    }
    return SERVER_SUCCESS;
}

ServerStatus eagleeye_pipeline_server_render(std::string server_key){
    // render key:
    std::string server_render_key = server_key + "/render";
    void* pipeline_render_obj = RegisterCenter::getInstance()->getObj(server_render_key);
    if(pipeline_render_obj == NULL){
        return SERVER_NOT_EXIST;
    }

    AnyPipeline* pipeline_render = (AnyPipeline*)pipeline_render_obj;
    pipeline_render->start();
    return SERVER_SUCCESS;
}

ServerStatus eagleeye_pipeline_server_stop(std::string server_key){
    RegisterCenter::getInstance()->destroyObj(server_key);
    return SERVER_SUCCESS;
}

void* eagleeye_create_node(std::string node_cls_name){
    AnyNode* op = CreateNode<>(node_cls_name);
    return op;
}

bool eagleeye_destroy_node(void* node_obj){
    if(node_obj == NULL){
        return false;
    }

    AnyNode* node_p = (AnyNode*)node_obj;
    delete node_p;
    return true;
}

void eagleeye_bind_node(void* node_obj, int node_port, void* sig){
    AnyNode* node_p = (AnyNode*)node_obj;
    node_p->setInputPort(sig, node_port);
}
}