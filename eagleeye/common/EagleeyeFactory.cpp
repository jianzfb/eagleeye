#include "eagleeye/common/EagleeyeFactory.h"
#include "eagleeye/processnode/Placeholder.h"
#include "eagleeye/processnode/AutoNode.h"
#include "eagleeye/common/EagleeyeFile.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeTime.h"
#include "eagleeye/common/EagleeyeNodeManager.h"
#include "eagleeye/processnode/YUVResizeNode.h"
#include "eagleeye/processnode/YUVConvertNode.h"
#include "eagleeye/processnode/CropNode.h"
#include "eagleeye/processnode/IdentityNode.h"
#include "eagleeye/processnode/ImageResizeNode.h"
#include "eagleeye/processnode/NNNode.h"
#include "eagleeye/processnode/ImageTransformNode.h"
#include "eagleeye/processnode/ImageSelect.h"
#include "eagleeye/processnode/ImageReadNode.h"
#include "eagleeye/processnode/ImageWriteNode.h"
#ifdef EAGLEEYE_OPENGL
#include "eagleeye/render/ImageShow.h"
#include "eagleeye/render/ImageBlend.h"
#include "eagleeye/render/HighlightShow.h"
#include "eagleeye/render/ShapeNode.h"
#endif

namespace eagleeye
{
AnyNode* __build_node(std::string node_name, std::string node_class, neb::CJsonObject node_param){
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
    else if(node_class == "PLACEHOLDER.RGB"|| node_class == "PLACEHOLDER.BGR"){
        Placeholder<ImageSignal<Array<unsigned char,3>>>* placeholder = 
                new Placeholder<ImageSignal<Array<unsigned char,3>>>();        
        if(node_class == "PLACEHOLDER.RGB"){    
            placeholder->setPlaceholderSignalType(EAGLEEYE_SIGNAL_RGB_IMAGE);
        }
        else{
            placeholder->setPlaceholderSignalType(EAGLEEYE_SIGNAL_BGR_IMAGE);
        }
        
        placeholder->setPlaceholderSource(EAGLEEYE_UNDEFINED_TARGET);
        placeholder->setUnitName(node_name.c_str());
        return placeholder;
    }
    else if(node_class == "PLACEHOLDER.RGBA"|| node_class == "PLACEHOLDER.BGRA"){
        Placeholder<ImageSignal<Array<unsigned char,4>>>* placeholder = 
                new Placeholder<ImageSignal<Array<unsigned char,4>>>();        
        if(node_class == "PLACEHOLDER.RGBA"){    
            placeholder->setPlaceholderSignalType(EAGLEEYE_SIGNAL_RGBA_IMAGE);
        }
        else{
            placeholder->setPlaceholderSignalType(EAGLEEYE_SIGNAL_BGRA_IMAGE);
        }
        
        placeholder->setPlaceholderSource(EAGLEEYE_UNDEFINED_TARGET);
        placeholder->setUnitName(node_name.c_str());
        return placeholder;
    }
    else if(node_class == "PLACEHOLDER.GRAY"){
        Placeholder<ImageSignal<unsigned char>>* placeholder = 
                new Placeholder<ImageSignal<unsigned char>>();        
        placeholder->setPlaceholderSignalType(EAGLEEYE_SIGNAL_GRAY_IMAGE);
        placeholder->setPlaceholderSource(EAGLEEYE_UNDEFINED_TARGET);
        placeholder->setUnitName(node_name.c_str());
        return placeholder;
    }
    else if(node_class == "VideoWriteNode"){
        return NULL;
    }
    else if(node_class == "ImageCanvasShow"){
        return NULL;
    }
    else if(node_class == "YUVResizeNode"){
        std::string resize_type;    // 忽略
        node_param.Get("resize_type", resize_type);
        float resize_scale = 1.0f;
        node_param.Get("resize_scale", resize_scale);

        // 优先resize_w,resize_h的设置
        int resize_w = 0;
        int resize_h = 0;
        node_param.Get("resize_w", resize_w);
        node_param.Get("resize_h", resize_h);

        YUVResizeNode* node = new YUVResizeNode(resize_w, resize_h, resize_scale);
        node->setUnitName(node_name.c_str());
        return node;
    }
    else if(node_class == "YUVConvertNode"){
        std::string convert_type;
        node_param.Get("convert_type", convert_type);
        YUVConvertType yuv_convert_type = NV21ToBGR;
        if(convert_type == "I420ToRGB"){
            yuv_convert_type = I420ToRGB;
        }
        else if(convert_type == "I420ToBGR"){
            yuv_convert_type = I420ToBGR;
        }
        else if(convert_type == "NV21ToRGB"){
            yuv_convert_type = NV21ToRGB;
        }
        else if(convert_type == "NV21ToBGR"){
            yuv_convert_type = NV21ToBGR;
        }
        else if(convert_type == "NV12ToRGB"){
            yuv_convert_type = NV12ToRGB;
        }
        else{
            yuv_convert_type = NV12ToBGR;
        }

        YUVConvertNode* node = new YUVConvertNode(yuv_convert_type);
        node->setUnitName(node_name.c_str());
        return node;
    }
    else if(node_class == "CropNode"){
        neb::CJsonObject region_obj;
        node_param.Get("region", region_obj);
        float x=0.0f; float y=0.0f; float w=0.0f; float h=0.0f;
        region_obj.Get(0, x);
        region_obj.Get(1, y);
        region_obj.Get(2, w);
        region_obj.Get(3, h);
        std::vector<float> region({x,y,w,h});

        CropNode* node = new CropNode(region);
        node->setUnitName(node_name.c_str());
        return node;
    }
    else if(node_class == "ImageResizeNode"){
        std::string resize_type;    // 忽略
        node_param.Get("resize_type", resize_type);
        float resize_scale = 1.0f;
        node_param.Get("resize_scale", resize_scale);

        // 优先resizew,resize_h的设置
        int resize_w = 0;
        int resize_h = 0;
        node_param.Get("resize_w", resize_w);
        node_param.Get("resize_h", resize_h);

        ImageResizeNode* node = new ImageResizeNode(resize_w, resize_h, resize_scale);
        node->setUnitName(node_name.c_str());
        return node;
    }
    else if(node_class == "IdentityNode"){
        IdentityNode* node = new IdentityNode();
        node->setUnitName(node_name.c_str());
        return node;
    }
    else if(node_class == "ImageTransformNode"){
        neb::CJsonObject region_obj;
        node_param.Get("region", region_obj);
        float x=0.0f; float y=0.0f; float w=0.0f; float h=0.0f;
        region_obj.Get(0, x);
        region_obj.Get(1, y);
        region_obj.Get(2, w);
        region_obj.Get(3, h);
        std::vector<float> region({x,y,w,h});

        neb::CJsonObject size_obj;
        node_param.Get("size", size_obj);
        int resize_h = 0; int resize_w = 0;
        size_obj.Get(0, resize_h);
        size_obj.Get(1, resize_w);
        std::vector<int> size({resize_h, resize_w});
        ImageTransformNode* node = new ImageTransformNode(region, size);
        node->setUnitName(node_name.c_str());
        return node;
    }
    else if(node_class == "ImageSelect"){
        ImageSelect* node = new ImageSelect();
        node->setUnitName(node_name.c_str());
        return node;
    }
    else if(node_class == "ImageRead"){
        ImageReadNode* node = new ImageReadNode();
        node->setUnitName(node_name.c_str());
        return node;
    }
    else if(node_class == "ImageWrite"){
        ImageWriteNode* node = new ImageWriteNode();
        node->setUnitName(node_name.c_str());
        return node;
    }
#ifdef EAGLEEYE_OPENGL    
    else if(node_class == "ImageShow"){
        ImageShow* node = new ImageShow();
        node->setUnitName(node_name.c_str());
        return node;
    }
    else if(node_class == "ImageBlend"){
        ImageBlend* node = new ImageBlend();
        node->setUnitName(node_name.c_str());
        return node;
    }
    else if(node_class == "HighlightShow"){
        HighlightShow* node = new HighlightShow();
        node->setUnitName(node_name.c_str());
        return node;
    }
    else if(node_class == "ShapeNode"){
        ShapeNode* node = new ShapeNode();
        node->setUnitName(node_name.c_str());
        return node;
    }
#endif
    AnyNode* node = NodeManager::get()->build(node_class, node_param);
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
    return NULL;
    // if(!isfileexist(json_file)){
    //     EAGLEEYE_LOGE("Pipeline json %s dont exist.", json_file);
    //     return NULL;
    // }

    // EAGLEEYE_LOGD("A");
    // std::ifstream i_file_handle;
    // i_file_handle.open(json_file);
    // i_file_handle.seekg(0, std::ios::end);
    // int file_buffer_size = i_file_handle.tellg();
    // i_file_handle.seekg(0, std::ios::beg);
    // char* file_buffer = new char[file_buffer_size+1];
    // memset(file_buffer, '\0', sizeof(char)*(file_buffer_size+1));
    // i_file_handle.read(file_buffer, file_buffer_size);
    // std::string content = file_buffer;
    // i_file_handle.close();
    // delete[] file_buffer;

    // EAGLEEYE_LOGD("B");
    // neb::CJsonObject pipeline_obj(content);
    // // 获得管线名称
    // std::string pipeline_name;
    // pipeline_obj.Get("name", pipeline_name);

    // EAGLEEYE_LOGD("C");
    // if(!AnyPipeline::isExist(pipeline_name.c_str())){
    //     // 注册管线
    //     const char* version = "default";
    //     const char* key = "";
    //     AnyPipeline::registerPipeline(pipeline_name.c_str(), version, key);
    // }    
    // EAGLEEYE_LOGD("D");
    // EAGLEEYE_LOGD("pp %s", pipeline_name.c_str());
    // AnyPipeline* pipeline = AnyPipeline::getInstance(pipeline_name.c_str());
    // if(pipeline == NULL){
    //     EAGLEEYE_LOGE("Couldnt get pipeline instance.");
    // }
    // EAGLEEYE_LOGD("E");

    // pipeline->setPipelineName(pipeline_name.c_str());
    // EAGLEEYE_LOGD("Pipeline name %s.", pipeline_name.c_str());
    // auto init_func = [&]() {
    //     // 获得管线配置信息
    //     neb::CJsonObject pipeline_config = pipeline_obj["pipeline"];

    //     // 获得节点列表
    //     neb::CJsonObject nodes_obj;
    //     pipeline_config.Get("node", nodes_obj);
    //     std::vector<std::string> nodes;
    //     for(int node_i=0; node_i<nodes_obj.GetArraySize(); ++node_i){
    //         std::string node_name;
    //         nodes_obj.Get(node_i, node_name);
    //         nodes.push_back(node_name);
    //         EAGLEEYE_LOGD("Node %s.", node_name.c_str());
    //     }

    //     // 获得节点配置信息并构建节点对象
    //     std::map<std::string, _NodeInfo> nodes_map;
    //     neb::CJsonObject nodes_attr;
    //     pipeline_config.Get("attribute", nodes_attr);
    //     for(int node_i=0; node_i<nodes.size(); ++node_i){
    //         if(!nodes_attr.GetKey(nodes[node_i])){
    //             EAGLEEYE_LOGD("Node %s attr dont exist", nodes[node_i].c_str());
    //             return false;
    //         }

    //         neb::CJsonObject node_attr;
    //         nodes_attr.Get(nodes[node_i], node_attr);
    //         int input_port = 0;
    //         int output_port = 0;
    //         std::string class_name = "";
    //         node_attr.Get("input", input_port);
    //         node_attr.Get("output", output_port);
    //         node_attr.Get("type", class_name);

    //         _NodeInfo node_info;
    //         node_info.input_port_num = input_port;
    //         node_info.output_port_num = output_port;
    //         node_info.class_name = class_name;

    //         nodes_map[nodes[node_i]] = node_info;

    //         AnyNode* node_ptr = __build_node(nodes[node_i], class_name, );
    //         if(node_ptr == NULL){
    //             EAGLEEYE_LOGD("Node %s (class %s) dont support.", nodes[node_i].c_str(), class_name.c_str());
    //             return false;
    //         }

    //         pipeline->add(node_ptr, nodes[node_i].c_str());
    //     }

    //     // 根据管线拓扑信息构建链接关系
    //     neb::CJsonObject topology;
    //     pipeline_config.Get("topology", topology);
    //     for(int node_i=0; node_i<nodes.size(); ++node_i){
    //         std::string node_name = nodes[node_i];

    //         if(!topology.KeyExist(node_name)){
    //             EAGLEEYE_LOGD("Node %s not in topology config.", node_name.c_str());
    //             return false;
    //         }

    //         neb::CJsonObject links_config;
    //         topology.Get(node_name, links_config);
    //         for(int link_i=0; link_i<links_config.GetArraySize(); ++link_i){
    //             neb::CJsonObject link_c;
    //             links_config.Get(link_i, link_c);

    //             std::string from_node_name;
    //             int from_node_port = 0;
                
    //             neb::CJsonObject from_c;
    //             link_c.Get("from", from_c);
    //             from_c.Get("node", from_node_name);
    //             from_c.Get("port", from_node_port);

    //             std::string to_node_name;
    //             int to_node_port = 0;
    //             neb::CJsonObject to_c;
    //             link_c.Get("to", to_c);
    //             to_c.Get("node", to_node_name);
    //             to_c.Get("port", to_node_port);

    //             pipeline->bind(node_name.c_str(), from_node_port, to_node_name.c_str(), to_node_port);
    //         }
    //     }

    //     return true;
    // };

    // pipeline->initialize(resource_folder, init_func, true);
    // return pipeline;
}

bool eagleeye_build_pipeline_from_json(AnyPipeline*pipeline, 
                                        const char* json_file,
                                        NodeBFuncType node_builder){
    if(!isfileexist(json_file)){
        EAGLEEYE_LOGE("Pipeline json %s dont exist.", json_file);
        return false;
    }

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

    neb::CJsonObject pipeline_obj(content);
    // 获得管线名称
    std::string pipeline_name;
    pipeline_obj.Get("name", pipeline_name);

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

    // 1.step 创建管线节点
    neb::CJsonObject nodes_attr;
    pipeline_config.Get("attribute", nodes_attr);
    std::vector<AnyNode*> node_ptrs(nodes.size());
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

        neb::CJsonObject node_param;
        node_attr.Get("param", node_param);
        
        // use in-build node factory
        AnyNode* node_ptr = __build_node(nodes[node_i], class_name, node_param);
        if(node_ptr == NULL && node_builder != nullptr){
            // use outside node factory
            node_builder(nodes[node_i],class_name, node_param, node_ptr, pipeline->resourceFolder());
        }

        if(node_ptr == NULL){
            EAGLEEYE_LOGD("Node %s (class %s) dont support.", nodes[node_i].c_str(), class_name.c_str());
            return false;
        }

        if(class_name == "NNNode"){
            neb::CJsonObject node_signal;
            node_attr.Get("signal", node_signal);

            neb::CJsonObject node_output_signals;
            node_signal.Get("output", node_output_signals);
            for(int i=0; i<node_output_signals.GetArraySize(); ++i){
                neb::CJsonObject cc;
                node_output_signals.Get(i, cc);

                std::string signal_type_str;
                cc.Get("type", signal_type_str);

                ((NNNode*)node_ptr)->makeOutputSignal(i, signal_type_str);
            }

        }

        node_ptrs[node_i] = node_ptr;
        pipeline->add(node_ptr, nodes[node_i].c_str());
    }


    // 2.step 创建管线节点拓扑关系
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

    // 3.step 刷新管线节点的信号类型
    for(int node_i=0; node_i<nodes.size(); ++node_i){
        neb::CJsonObject node_attr;
        nodes_attr.Get(nodes[node_i], node_attr);

        neb::CJsonObject node_signal;
        node_attr.Get("signal", node_signal);

        neb::CJsonObject node_output_signals;
        node_signal.Get("output", node_output_signals);
        for(int i=0; i<node_output_signals.GetArraySize(); ++i){
            neb::CJsonObject cc;
            node_output_signals.Get(i, cc);

            std::string signal_type_str;
            cc.Get("type", signal_type_str);

            SignalType st = EAGLEEYE_UNDEFINED_SIGNAL;
            if(signal_type_str == "EAGLEEYE_SIGNAL_IMAGE"){
                st = EAGLEEYE_SIGNAL_IMAGE;
            }
            else if(signal_type_str == "EAGLEEYE_SIGNAL_RGB_IMAGE"){
                st = EAGLEEYE_SIGNAL_RGB_IMAGE;
            }
            else if(signal_type_str == "EAGLEEYE_SIGNAL_RGBA_IMAGE"){
                st = EAGLEEYE_SIGNAL_RGBA_IMAGE;
            }
            else if(signal_type_str == "EAGLEEYE_SIGNAL_BGR_IMAGE"){
                st = EAGLEEYE_SIGNAL_BGR_IMAGE;
            }
            else if(signal_type_str == "EAGLEEYE_SIGNAL_BGRA_IMAGE"){
                st = EAGLEEYE_SIGNAL_BGRA_IMAGE;
            }
            else if(signal_type_str == "EAGLEEYE_SIGNAL_GRAY_IMAGE"){
                st = EAGLEEYE_SIGNAL_GRAY_IMAGE;
            }
            else if(signal_type_str == "EAGLEEYE_SIGNAL_RECT"){
                st = EAGLEEYE_SIGNAL_RECT;
            }
            else if(signal_type_str == "EAGLEEYE_SIGNAL_LINE"){
                st = EAGLEEYE_SIGNAL_LINE;
            }   
            else if(signal_type_str == "EAGLEEYE_SIGNAL_POINT"){
                st = EAGLEEYE_SIGNAL_POINT;
            }   
            else if(signal_type_str == "EAGLEEYE_SIGNAL_FILE"){
                st = EAGLEEYE_SIGNAL_FILE;
            }                                                
            else if(signal_type_str == "EAGLEEYE_SIGNAL_MASK"){
                st = EAGLEEYE_SIGNAL_MASK;
            }   
            else if(signal_type_str == "EAGLEEYE_SIGNAL_STATE"){
                st = EAGLEEYE_SIGNAL_STATE;
            }     
            else if(signal_type_str == "EAGLEEYE_SIGNAL_LANDMARK"){
                st = EAGLEEYE_SIGNAL_LANDMARK;
            }     
            else if(signal_type_str == "EAGLEEYE_SIGNAL_DET"){
                st = EAGLEEYE_SIGNAL_DET;
            }     
            else if(signal_type_str == "EAGLEEYE_SIGNAL_DET_EXT"){
                st = EAGLEEYE_SIGNAL_DET_EXT;
            }    
            else if(signal_type_str == "EAGLEEYE_SIGNAL_TRACKING"){
                st = EAGLEEYE_SIGNAL_TRACKING;
            }  
            else if(signal_type_str == "EAGLEEYE_SIGNAL_CLS"){
                st = EAGLEEYE_SIGNAL_CLS;
            } 
            else if(signal_type_str == "EAGLEEYE_SIGNAL_POS_2D"){
                st = EAGLEEYE_SIGNAL_POS_2D;
            } 
            else if(signal_type_str == "EAGLEEYE_SIGNAL_POS_3D"){
                st = EAGLEEYE_SIGNAL_POS_3D;
            }
            else if(signal_type_str == "EAGLEEYE_SIGNAL_YUV_IMAGE"){
                st = EAGLEEYE_SIGNAL_YUV_IMAGE;
            }                                                                                         
            else if(signal_type_str == "EAGLEEYE_SIGNAL_STRING"){
                st = EAGLEEYE_SIGNAL_STRING;
            }   
            else if(signal_type_str == "EAGLEEYE_SIGNAL_SWITCH"){
                st = EAGLEEYE_SIGNAL_SWITCH;
            }  
            else if(signal_type_str == "EAGLEEYE_SIGNAL_DATA"){
                st = EAGLEEYE_SIGNAL_DATA;
            }               
            node_ptrs[node_i]->getOutputPort(i)->setSignalType(st);
        }
    }

    return true;
}
} // namespace eagleeye
