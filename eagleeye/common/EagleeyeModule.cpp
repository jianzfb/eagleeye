#include "eagleeye/common/EagleeyeModule.h"
#include "eagleeye/common/EagleeyePy.h"
#include "eagleeye/framework/pipeline/AnyPipeline.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeLog.h"
#include <dlfcn.h>
#include <dirent.h>

namespace eagleeye{
void _getAllFileFromDirectory(std::string path, std::vector<std::string> &files) {
        DIR *dp = NULL;
        struct dirent *dirp;
        if ((dp = opendir(path.c_str())) == NULL) {
            assert("can't open dir");
        }
        while ((dirp = readdir(dp)) != NULL) {
            std::string file_name = dirp->d_name;
            if(endswith(file_name, ".so")){
                files.push_back(file_name);
                // std::cout << dirp->d_name << std::endl;
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
    // 加载所有插件，发现注册模块
    EAGLEEYE_LOGD("traverse to find all plugin in %s",plugin_folder);
    std::vector<std::string> plugin_list;
    _getAllFileFromDirectory(plugin_folder, plugin_list);
    for(int index=0; index<plugin_list.size(); ++index){
        std::string plugin_path = std::string(plugin_folder) + "/" + plugin_list[index];
        // 加载so
        EAGLEEYE_LOGD("load plugin %s from path %s", plugin_list[index].c_str(), plugin_path.c_str());
        void* handle = dlopen(plugin_path.c_str(), RTLD_LAZY);
        if(!handle){        
            EAGLEEYE_LOGD("dlopen error, message (%s)",dlerror());
            continue;
        }

        // 加载注册及初始化函数
        EAGLEEYE_LOGD("get register plugin func");
        REGISTER_PLUGIN_FUNC plugin_register_func = NULL; 
        INITIALIZE_PLUGIN_FUNC plugin_initialize_func = NULL;
        plugin_register_func = (REGISTER_PLUGIN_FUNC)dlsym(handle, "eagleeye_register_plugin_pipeline");
        if(!plugin_register_func){
            EAGLEEYE_LOGD("dlsym error, message (%s)",dlerror());
            dlclose(handle);
            continue;
        }        

        EAGLEEYE_LOGD("finish get register plugin func");
        const char* plugin_name = plugin_register_func();
        if(plugin_name == NULL){
            EAGLEEYE_LOGD("plugin %s fail to register");
            continue;
        }

        EAGLEEYE_LOGD("get initialize plugin func");
        plugin_initialize_func = (INITIALIZE_PLUGIN_FUNC)dlsym(handle, "eagleeye_pipeline_initialize");
        if(!plugin_initialize_func){
            EAGLEEYE_LOGD("dlsym error, message (%s)",dlerror());
            dlclose(handle);
            continue;
        }        

        EAGLEEYE_LOGD("finish initialize plugin func");
        AnyPipeline* pipeline = AnyPipeline::getInstance(plugin_name);
        if(pipeline == NULL){
            EAGLEEYE_LOGD("couldnt get pipeline instance");
        }
        pipeline->setInitFunc(plugin_initialize_func);

        // add to 
        m_registed_plugins[std::string(plugin_name)] = std::make_pair(handle,plugin_initialize_func);
    }

    // 获得所有注册的pipeline
    AnyPipeline::getRegistedPipelines(pipeline_names);
    if(pipeline_names.size() == 0){
        EAGLEEYE_LOGD("found 0 pipeline plugin");
        return false;
    }
    
    EAGLEEYE_LOGD("found %d pipeline plugin", pipeline_names.size());
    return true;
}

bool eagleeye_release_module(){
    EAGLEEYE_LOGD("enter release plugin handler");
    std::map<std::string, std::pair<void*,INITIALIZE_PLUGIN_FUNC>>::iterator iter,iend(m_registed_plugins.end());

    for(iter=m_registed_plugins.begin(); iter != iend; ++iter){
        EAGLEEYE_LOGD("release so %s", iter->first.c_str());
        if(iter->second.first != NULL){
             dlclose(iter->second.first);
        }
    }
    
    m_registed_plugins.clear();
    EAGLEEYE_LOGD("finish release plugin handler");
    return true;
}

// #ifdef SUPPORT_STATIC_PIPELINE_PLUGIN
// extern "C" {
//     const char* eagleeye_register_plugin_pipeline();
//     void eagleeye_pipeline_initialize();
// }
// #endif

// bool eagleeye_registed_pipeline(std::string& pipeline_name){
//     #ifdef SUPPORT_STATIC_PIPELINE_PLUGIN
//     pipeline_name = eagleeye_register_plugin_pipeline();
//     INITIALIZE_PLUGIN_FUNC pipelne_ini_func = eagleeye_pipeline_initialize;
//     m_registed_plugins[pipeline_name] = std::make_pair(NULL,plugin_initialize_func);
//     AnyPipeline::getInstance(pipeline_name.c_str())->setInitFunc(pipelne_ini_func);
//     return true;
//     #endif

//     return false;
// }

bool eagleeye_custom_add_pipeline(REGISTER_PLUGIN_FUNC register_func, INITIALIZE_PLUGIN_FUNC init_func){
    const char* pipeline_name = register_func();
    if(pipeline_name == NULL){
        EAGLEEYE_LOGE("resiger fail");
        return false;
    }

    m_registed_plugins[std::string(pipeline_name)]=std::pair<void*,INITIALIZE_PLUGIN_FUNC>(NULL,init_func);
    return true;
}

bool eagleeye_pipeline_initialize(const char* pipeline_name, const char* config_folder){
    // initialize
    EAGLEEYE_LOGD("enter eagleeye_pipeline_initialize");
    AnyPipeline::getInstance(pipeline_name)->setInitFunc(m_registed_plugins[std::string(pipeline_name)].second);
    AnyPipeline::getInstance(pipeline_name)->setPipelineName(pipeline_name);
    AnyPipeline::getInstance(pipeline_name)->initialize(config_folder);
    EAGLEEYE_LOGD("finish eagleeye_pipeline_initialize");

    return true;
}

bool eagleeye_pipeline_load_config(const char* pipeline_name, const char* config_file){
    EAGLEEYE_LOGD("load pipeline %s from configure file %s", pipeline_name, config_file);
    AnyPipeline::getInstance(pipeline_name)->loadConfigure(config_file);
    return true;
}

bool eagleeye_pipeline_save_config(const char* pipeline_name, const char* config_file){
    EAGLEEYE_LOGD("save pipeline %s to configure file %s", pipeline_name, config_file);
    AnyPipeline::getInstance(pipeline_name)->saveConfigure(config_file);
    return true;
}

bool eagleeye_pipeline_release(const char* pipeline_name){
    EAGLEEYE_LOGD("enter eagleeye_pipeline_release %s", pipeline_name);
    AnyPipeline::releasePipeline(pipeline_name);
    EAGLEEYE_LOGD("finish eagleeye_pipeline_release %s", pipeline_name);
    return true;
}

bool eagleeye_pipeline_version(const char* pipeline_name, char* pipeline_version){
    const char* version = AnyPipeline::getInstance(pipeline_name)->getPipelineVersion();
    memcpy(pipeline_version, version, sizeof(char)*strlen(version) + 1);
    return true;
}

bool eagleeye_register_pipeline(const char* pipeline, const char* version, const char* key){
    EAGLEEYE_LOGD("register %s", pipeline);
    return AnyPipeline::registerPipeline(pipeline, version, key);
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
                                        std::vector<std::string>& input_sources){
    // get pipeline input   
    AnyPipeline::getInstance(pipeline_name)->getPipelineInputs(input_nodes, input_types, input_sources);
    return true;
}

bool eagleeye_pipeline_get_output_config(const char* pipeline_name,
                                         std::vector<std::string>& output_nodes,
                                         std::vector<std::string>& output_types,
                                         std::vector<std::string>& output_targets){
    // get pipeline output
    AnyPipeline::getInstance(pipeline_name)->getPipelineOutputs(output_nodes, output_types, output_targets);
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
                                 const int* data_size, 
                                 const int data_dims,
                                 const int data_type){
    AnyPipeline::getInstance(pipeline_name)->setInput(node_name, data, data_size, data_dims, data_type);
    return true;
}

bool eagleeye_pipeline_get_node_output(const char* pipeline_name,
                                  const char* node_name, 
                                  void*& data, 
                                  int* data_size, 
                                  int& data_dims,
                                  int& data_type){
    EAGLEEYE_LOGD("%s pipeline get data from node %s", pipeline_name, node_name);
    AnyPipeline::getInstance(pipeline_name)->getNodeOutput(node_name, data, data_size, data_dims, data_type);
    return true;
}

bool eagleeye_pipeline_get_output(const char* pipeline_name,
                                  const char* node_name, 
                                  void*& data, 
                                  int* data_size, 
                                  int& data_dims,
                                  int& data_type){
    EAGLEEYE_LOGD("%s pipeline get data from node %s", pipeline_name, node_name);
    AnyPipeline::getInstance(pipeline_name)->getOutput(node_name, data, data_size, data_dims, data_type);
    return true;
}

bool eagleeye_pipeline_run(const char* pipeline_name){
    std::string s_pipeline_name = pipeline_name;
    if(s_pipeline_name.find("/") != std::string::npos){
        std::vector<std::string> kterms = split(s_pipeline_name, "/");
        std::string pn = kterms[0];
        std::string nn = kterms[1];
        
        bool r = AnyPipeline::getInstance(pn.c_str())->start(nn.c_str());
        return r;
    }

    bool result = AnyPipeline::getInstance(pipeline_name)->start();
    return result;
}

bool eagleeye_pipeline_isready(const char* pipeline_name, int& is_ready){
    AnyPipeline::getInstance(pipeline_name)->isReady(is_ready);
    return true;
}

bool eagleeye_pipeline_reset(const char* pipeline_name){
    EAGLEEYE_LOGD("enter pipeline %s reset", pipeline_name);
    AnyPipeline::getInstance(pipeline_name)->reset();
    EAGLEEYE_LOGD("finish pipeline %s reset", pipeline_name);

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

#ifdef EAGLEEYE_PYTHON_MODULE
// initialize pipeline
EAGLEEYE_PYTHON_FUNC_D(eagleeye_pipeline_initialize, bool);

// run pipeline
EAGLEEYE_PYTHON_FUNC_D(eagleeye_pipeline_run, bool);

// get pipeline input nodes configure
boost::python::list eagleeye_pipeline_get_input_config_py(){
    std::vector<std::string> input_nodes;
    std::vector<std::string> input_types;
    std::vector<std::string> input_sources;

    eagleeye_pipeline_get_input_config(input_nodes,
                                        input_types,
                                        input_sources);
    boost::python::list py_input_nodes = std_vector_to_py_list<std::string>(input_nodes);
    boost::python::list py_input_types = std_vector_to_py_list<std::string>(input_types);
    boost::python::list py_input_sources = std_vector_to_py_list<std::string>(input_sources);

    boost::python::list triple_content;
    triple_content.append(py_input_nodes);
    triple_content.append(py_input_types);
    triple_content.append(py_input_sources);

    return triple_content;
}

// get pipeline output nodes configure
boost::python::list eagleeye_pipeline_get_output_config_py(){
    std::vector<std::string> output_nodes;
    std::vector<std::string> output_types;
    std::vector<std::string> output_sources;

    eagleeye_pipeline_get_output_config(output_nodes,
                                        output_types,
                                        output_sources);
    boost::python::list py_output_nodes = std_vector_to_py_list<std::string>(output_nodes);
    boost::python::list py_output_types = std_vector_to_py_list<std::string>(output_types);
    boost::python::list py_output_sources = std_vector_to_py_list<std::string>(output_sources);

    boost::python::list triple_content;
    triple_content.append(py_output_nodes);
    triple_content.append(py_output_types);
    triple_content.append(py_output_sources);

    return triple_content;
}

// get pipeline monitors configure
boost::python::list eagleeye_pipeline_get_monitor_config_py(){
    std::vector<std::string> monitor_names;
    std::vector<int> monitor_types;
    std::vector<std::string> monitor_range;
    eagleeye_pipeline_get_monitor_config(monitor_names, monitor_types, monitor_range);

    boost::python::list py_monitor_nodes = std_vector_to_py_list<std::string>(monitor_names);
    boost::python::list py_monitor_types = std_vector_to_py_list<int>(monitor_types);
    boost::python::list py_monitor_range = std_vector_to_py_list<std::string>(monitor_range);

    boost::python::list triple_content;
    triple_content.append(py_monitor_nodes);
    triple_content.append(py_monitor_types);
    triple_content.append(py_monitor_range);

    return triple_content;
}

// set pipeline input data
void eagleeye_pipeline_set_input_py(const char* node_name, boost::python::object obj){
    PyObject* pobj = obj.ptr();
		Py_buffer pybuf;
	    if(PyObject_GetBuffer(pobj, &pybuf, PyBUF_C_CONTIGUOUS)!=-1){
	        void *buf = pybuf.buf;
	        Py_ssize_t *shape = pybuf.shape;
	        int ndim = pybuf.ndim;
            int data_size[3] = {0,0,0};
            switch(ndim){
                case 1:
                    data_size[0] = shape[0];
                    break;
                case 2:
                    data_size[0] = shape[0];
                    data_size[1] = shape[1];
                    break;
                case 3:
                    data_size[0] = shape[0];
                    data_size[1] = shape[1];
                    data_size[2] = shape[2];
                    break;
                default:
                    ndim = 0;
                    break;
            }

	        if(ndim >= 3 || ndim == 0){
	        	PyBuffer_Release(&pybuf);
	        }
            eagleeye_pipeline_set_input(node_name, buf, data_size, ndim);
	        PyBuffer_Release(&pybuf);
	    }
}

// get pipeline output data
PyObject* eagleeye_pipeline_get_output_py(const char* node_name){    
    // 1.step get pipeline output by node
    void* data;
    int* data_size;
    int data_dims;
    int data_type;
    eagleeye_pipeline_get_output(node_name, data, data_size, data_dims, data_type);
    
    // 2.step parse data and return numpy
    std::vector<std::string> output_nodes;
    std::vector<std::string> output_types;
    std::vector<std::string> output_targets;
    eagleeye_pipeline_get_output_config(output_nodes,output_types,output_targets);
    std::string node_type;
    for(int index=0; index<output_nodes.size(); ++index){
        if(output_nodes[index] == node_name){
            node_type = output_types[index];
        }
    }
    if(node_type == ""){
        return NULL;
    }

    std::string data_type;
    if(node_type.find("/") == std::string::npos){
        data_type = "RGB";
    }
    else{
        std::string separator = "/";
        std::vector<std::string> terms = split(node_type, separator);
        data_type = terms[1];
    }

    // 
    PyObject* out_matrix = NULL;
    if(data_type == "float"){
        npy_intp _sizes[2];
        _sizes[0] = data_size[0];
        _sizes[1] = data_size[1];
        out_matrix = PyArray_SimpleNew(2, _sizes, NPY_FLOAT);
        void* ptr_out;
        int count;
        PyArrayObject* output_contigous_array;
        reference_contiguous_array(out_matrix, output_contigous_array, ptr_out, count);
        memcpy(ptr_out, data, sizeof(float)*_sizes[0]*_sizes[1]);
        Py_DECREF(output_contigous_array);
    }
    else if(data_type == "double"){
        npy_intp _sizes[2];
        _sizes[0] = data_size[0];
        _sizes[1] = data_size[1];
        out_matrix = PyArray_SimpleNew(2, _sizes, NPY_DOUBLE);
        void* ptr_out;
        int count;
        PyArrayObject* output_contigous_array;
        reference_contiguous_array(out_matrix, output_contigous_array, ptr_out, count);
        memcpy(ptr_out, data, sizeof(double)*_sizes[0]*_sizes[1]);
        Py_DECREF(output_contigous_array);
    }
    else if(data_type == "INT"){
        npy_intp _sizes[2];
        _sizes[0] = data_size[0];
        _sizes[1] = data_size[1];
        out_matrix = PyArray_SimpleNew(2, _sizes, NPY_INT32);
        void* ptr_out;
        int count;
        PyArrayObject* output_contigous_array;
        reference_contiguous_array(out_matrix, output_contigous_array, ptr_out, count);
        memcpy(ptr_out, data, sizeof(int)*_sizes[0]*_sizes[1]);
        Py_DECREF(output_contigous_array);
    }
    else if(data_type == "RGB"){
        npy_intp _sizes[3];
        _sizes[0] = data_size[0];
        _sizes[1] = data_size[1];
        _sizes[2] = data_size[2];
        out_matrix = PyArray_SimpleNew(3, _sizes, NPY_UINT8);
        void* ptr_out;
        int count;
        PyArrayObject* output_contigous_array;
        reference_contiguous_array(out_matrix, output_contigous_array, ptr_out, count);
        memcpy(ptr_out, data, sizeof(unsigned char)*_sizes[0]*_sizes[1]*_sizes[2]);
        Py_DECREF(output_contigous_array);
    }
    else{
        // unsigned char
        npy_intp _sizes[2];
        _sizes[0] = data_size[0];
        _sizes[1] = data_size[1];
        out_matrix = PyArray_SimpleNew(2, _sizes, NPY_UINT8);
        void* ptr_out;
        int count;
        PyArrayObject* output_contigous_array;
        reference_contiguous_array(out_matrix, output_contigous_array, ptr_out, count);
        memcpy(ptr_out, data, sizeof(unsigned char)*_sizes[0]*_sizes[1]);
        Py_DECREF(output_contigous_array);
    }
 
    return out_matrix;
}

void eagleeye_pipeline_set_param_py(const char* node_name, const char* param_name, float value){
    std::string node_name_str = node_name;
    std::string param_name_str = param_name;

    // get monitor config
    std::vector<std::string> monitor_names;
    std::vector<int> monitor_types;
    std::vector<std::string> monitor_range;
    eagleeye_pipeline_get_monitor_config(monitor_names,monitor_types,monitor_range);
    std::string query_name = node_name_str+"/"+param_name_str;
    int query_type = 0;
    for(int index=0; index<monitor_names.size(); ++index){
        if(query_name == monitor_names[index]){
            query_type = monitor_types[index];
        }
    }

    // set monitor value
    int value_int = 0; 
    float value_float = 0.0f;
    switch(query_type){
        case 0:
            // int
            value_int = (int)value;
            eagleeye_pipeline_set_param(node_name,param_name,&value_int);
            break;
        case 1:
            // float
            value_float = (float)value;
            eagleeye_pipeline_set_param(node_name,param_name,&value_float);
            break;
        default:
            break;
    }    
}

EAGLEEYE_PYTHON_MODULE(pipeline){
    EAGLEEYE_PYTHON_INIT();
    EAGLEEYE_PYTHON_DEF(pipeline_initialize,eagleeye_pipeline_initialize);
    EAGLEEYE_PYTHON_DEF(pipeline_run, eagleeye_pipeline_run);
    EAGLEEYE_PYTHON_DEF(pipeline_get_input_config,eagleeye_pipeline_get_input_config);
    EAGLEEYE_PYTHON_DEF(pipeline_get_output_config,eagleeye_pipeline_get_output_config);
    EAGLEEYE_PYTHON_DEF(pipeline_get_monitor_config,eagleeye_pipeline_get_monitor_config);
    EAGLEEYE_PYTHON_DEF(pipeline_set_input,eagleeye_pipeline_set_input);
    EAGLEEYE_PYTHON_DEF(pipeline_get_output,eagleeye_pipeline_get_output);
    EAGLEEYE_PYTHON_DEF(pipeline_set_param,eagleeye_pipeline_set_param);
}
#endif
}