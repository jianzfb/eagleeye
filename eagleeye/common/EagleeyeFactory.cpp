#include "eagleeye/common/EagleeyeFactory.h"
#include "eagleeye/processnode/Placeholder.h"
#include "eagleeye/processnode/AutoNode.h"
#include "eagleeye/common/EagleeyeFile.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeTime.h"
#include "eagleeye/common/EagleeyeNodeManager.h"
#include "eagleeye/processnode/YUVResizeNode.h"
#include "eagleeye/processnode/YUVConvertNode.h"
#include "eagleeye/render/ImageShow.h"

namespace eagleeye
{
AnyNode* __build_node(std::string node_class, std::string node_name){
    // CAMERA.IMAGE，CAMERA, GALLERY.IMAGE, GALLERY.VIDEO
    if(node_class == "CAMERA.IMAGE" || node_class == "CAMERA" || node_class == "GALLERY.IMAGE" || node_class == "GALLERY.VIDEO"){
        // 使用placeholder
        if(node_class == "CAMERA.IMAGE"){
            Placeholder<YUVSignal>* placeholder = 
                new Placeholder<YUVSignal>();
            placeholder->setPlaceholderSignalType(EAGLEEYE_SIGNAL_YUV_IMAGE);
            placeholder->setPlaceholderSource(EAGLEEYE_CAPTURE_PREVIEW_IMAGE);
            placeholder->setUnitName("placeholder");
            return placeholder;
        }
        else if(node_class == "CAMERA"){
            Placeholder<YUVSignal>* placeholder = 
                new Placeholder<YUVSignal>();
            placeholder->setPlaceholderSignalType(EAGLEEYE_SIGNAL_YUV_IMAGE);
            placeholder->setPlaceholderSource(EAGLEEYE_CAPTURE_PREVIEW_IMAGE);
            placeholder->setUnitName("placeholder");
            return placeholder;
        }
        else if(node_class == "GALLERY.IMAGE"){
            Placeholder<StringSignal>* placeholder = new Placeholder<StringSignal>();
            placeholder->setPlaceholderSignalType(EAGLEEYE_SIGNAL_FILE);
            placeholder->setUnitName("placeholder");
            return placeholder;            
        }
        else if(node_class == "GALLERY.VIDEO"){
            Placeholder<StringSignal>* placeholder = new Placeholder<StringSignal>();
            placeholder->setPlaceholderSignalType(EAGLEEYE_SIGNAL_FILE);
            placeholder->setUnitName("placeholder");
            return placeholder;   
        }
    }
    else if(node_class == "VideoWriteNode"){
        return NULL;
    }
    else if(node_class == "ImageCanvasShow"){
        return NULL;
    }
    else if(node_class == "YUVResizeNode"){
        return new YUVResizeNode();
    }
    else if(node_class == "YUVConvertNode"){
        return new YUVConvertNode();
    }
    else if(node_class == "ImageShow"){
        return new ImageShow();
    }

    AnyNode* node = NodeManager::get()->build(node_class);
    if(node != NULL){
        node->setUnitName(node_name.c_str());
    }

    return node;
}

struct _NodeInfo{
    int input_port_num;
    int output_port_num;
    std::string class_name;
};


AnyPipeline* eagleeye_build_pipeline_from_json(const char* json_file, const char* resource_folder){
    if(!isfileexist(json_file)){
        EAGLEEYE_LOGE("Pipeline json %s dont exist.", json_file);
        return NULL;
    }

    EAGLEEYE_LOGD("A");
    std::ifstream i_file_handle;
    i_file_handle.open(json_file);
    i_file_handle.seekg(0, std::ios::end);
    int file_buffer_size = i_file_handle.tellg();
    i_file_handle.seekg(0, std::ios::beg);
    char* file_buffer = new char[file_buffer_size+1];
    memset(file_buffer, '\0', sizeof(char)*(file_buffer_size+1));
    i_file_handle.read(file_buffer, file_buffer_size);
    std::string content = file_buffer;
    i_file_handle.close();
    delete[] file_buffer;

    EAGLEEYE_LOGD("B");
    neb::CJsonObject pipeline_obj(content);
    // 获得管线名称
    std::string pipeline_name;
    pipeline_obj.Get("name", pipeline_name);

    EAGLEEYE_LOGD("C");
    if(!AnyPipeline::isExist(pipeline_name.c_str())){
        // 注册管线
        const char* version = "default";
        const char* key = "";
        AnyPipeline::registerPipeline(pipeline_name.c_str(), version, key);
    }    
    EAGLEEYE_LOGD("D");
    EAGLEEYE_LOGD("pp %s", pipeline_name.c_str());
    AnyPipeline* pipeline = AnyPipeline::getInstance(pipeline_name.c_str());
    if(pipeline == NULL){
        EAGLEEYE_LOGE("Couldnt get pipeline instance.");
    }
    EAGLEEYE_LOGD("E");

    pipeline->setPipelineName(pipeline_name.c_str());
    EAGLEEYE_LOGD("Pipeline name %s.", pipeline_name.c_str());
    auto init_func = [&]() {
        // 获得管线配置信息
        neb::CJsonObject pipeline_config = pipeline_obj["pipeline"];

        // 获得节点列表
        neb::CJsonObject nodes_obj;
        pipeline_config.Get("node", nodes_obj);
        std::vector<std::string> nodes;
        for(int node_i=0; node_i<nodes_obj.GetArraySize(); ++node_i){
            std::string node_name;
            nodes_obj.Get(node_i, node_name);
            nodes.push_back(node_name);
            EAGLEEYE_LOGD("Node %s.", node_name.c_str());
        }

        // 获得节点配置信息并构建节点对象
        std::map<std::string, _NodeInfo> nodes_map;
        neb::CJsonObject nodes_attr;
        pipeline_config.Get("attribute", nodes_attr);
        for(int node_i=0; node_i<nodes.size(); ++node_i){
            if(!nodes_attr.GetKey(nodes[node_i])){
                EAGLEEYE_LOGD("Node %s attr dont exist", nodes[node_i].c_str());
                return false;
            }

            neb::CJsonObject node_attr;
            nodes_attr.Get(nodes[node_i], node_attr);
            int input_port = 0;
            int output_port = 0;
            std::string class_name = "";
            node_attr.Get("input", input_port);
            node_attr.Get("output", output_port);
            node_attr.Get("type", class_name);

            _NodeInfo node_info;
            node_info.input_port_num = input_port;
            node_info.output_port_num = output_port;
            node_info.class_name = class_name;

            nodes_map[nodes[node_i]] = node_info;

            AnyNode* node_ptr = __build_node(class_name, nodes[node_i]);
            if(node_ptr == NULL){
                EAGLEEYE_LOGD("Node %s (class %s) dont support.", nodes[node_i].c_str(), class_name.c_str());
                return false;
            }

            pipeline->add(node_ptr, nodes[node_i].c_str());
        }

        // 根据管线拓扑信息构建链接关系
        neb::CJsonObject topology;
        pipeline_config.Get("topology", topology);
        for(int node_i=0; node_i<nodes.size(); ++node_i){
            std::string node_name = nodes[node_i];

            if(!topology.KeyExist(node_name)){
                EAGLEEYE_LOGD("Node %s not in topology config.", node_name.c_str());
                return false;
            }

            neb::CJsonObject links_config;
            topology.Get(node_name, links_config);
            for(int link_i=0; link_i<links_config.GetArraySize(); ++link_i){
                neb::CJsonObject link_c;
                links_config.Get(link_i, link_c);

                std::string from_node_name;
                int from_node_port = 0;
                
                neb::CJsonObject from_c;
                link_c.Get("from", from_c);
                from_c.Get("node", from_node_name);
                from_c.Get("port", from_node_port);

                std::string to_node_name;
                int to_node_port = 0;
                neb::CJsonObject to_c;
                link_c.Get("to", to_c);
                to_c.Get("node", to_node_name);
                to_c.Get("port", to_node_port);

                pipeline->bind(node_name.c_str(), from_node_port, to_node_name.c_str(), to_node_port);
            }
        }

        return true;
    };

    pipeline->initialize(resource_folder, init_func, true);
    return pipeline;
}
} // namespace eagleeye
