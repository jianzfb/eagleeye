#include "eagleeye/common/EagleeyeFactory.h"
#include "eagleeye/processnode/Placeholder.h"
#include "eagleeye/processnode/Add.h"
#include "eagleeye/processnode/AutoNode.h"

namespace eagleeye
{
AnyNode* __build_placeholder(neb::CJsonObject config){
    int queue_mode=0;
    config["config"].Get("queue_mode", queue_mode);

    std::string input_data_type;
    config["input"][0].Get("data_type", input_data_type);

    std::string input_category;
    config["input"][0].Get("category", input_category);

    // ImageSignal<Array<unsigned char, 3>>
    if(input_category == "SIGNAL_IMAGE"){
        if(input_data_type == "RGB" || input_data_type == "BGR"){
            Placeholder<ImageSignal<Array<unsigned char, 3>>>* p = 
                new Placeholder<ImageSignal<Array<unsigned char, 3>>>(bool(queue_mode));
        }
        else if(input_data_type == "GRAY" || input_data_type == "UCHAR"){
            Placeholder<ImageSignal<unsigned char>>* p = 
                new Placeholder<ImageSignal<unsigned char>>(bool(queue_mode));
        }
    }
    else if(input_category == "SIGNAL_RECT" || 
            input_category == "SIGNAL_LINE" || 
            input_category == "SIGNAL_POINT"){
        if(input_data_type == "INT"){
            Placeholder<ImageSignal<int>>* p = 
                new Placeholder<ImageSignal<int>>(bool(queue_mode));
        }
    }
    else if(input_category == "SIGNAL_MASK"){
        Placeholder<ImageSignal<unsigned char>>* p = 
            new Placeholder<ImageSignal<unsigned char>>(bool(queue_mode));
    }
    else if(input_category == "ADVANCED_SIGNAL_DET"){
        Placeholder<ImageSignal<float>>* p = 
            new Placeholder<ImageSignal<float>>(bool(queue_mode));
    }

    return NULL;
}    

AnyNode* __build_add(neb::CJsonObject config){
    std::string input_data_type;
    config["input"][0].Get("data_type", input_data_type);

    std::string input_category;
    config["input"][0].Get("category", input_category);

    std::string output_data_type;
    config["output"][0].Get("data_type", output_data_type);

    std::string output_category;
    config["output"][0].Get("category", output_category);

    if(input_category != "SIGNAL_IMAGE" || output_category != "SIGNAL_IMAGE"){
        return NULL;
    }
    if(input_data_type == "UCHAR"){
        return new Add<ImageSignal<unsigned char>, ImageSignal<unsigned char>>();
    }
    else if(input_data_type == "INT"){
        return new Add<ImageSignal<int>, ImageSignal<int>>();
    }
    else if(input_data_type == "FLOAT"){
        return new Add<ImageSignal<float>, ImageSignal<float>>();
    }

    return NULL;
}

AnyNode* __build_auto_node(neb::CJsonObject config, std::function<AnyNode*()> option_func){
    return new AutoNode(option_func);
}

AnyNode* eagleeye_node_factory(neb::CJsonObject config, std::function<AnyNode*()> option_func){
    // 获得节点类型
    std::string node_type;
    config.Get("type", node_type);    

    // // 创建节点
    // if(node_type == "Placeholder"){
    //     return __build_placeholder(config);
    // }
    // else if(node_type == "Add"){
    //     return __build_add(config);
    // }
    // else if(node_type == "AutoNode"){
    //     return __build_auto_node(option_func);
    // }
    // else{
    //     return NULL;
    // }

    return NULL;
}    

void __placeholder_info(neb::CJsonObject& placeholder_config){
    // placeholder_config.Add();
}

void eagleeye_get_nodes_config(neb::CJsonObject& config){
    
    // 1.step placeholder


    // 2.step Add


}
} // namespace eagleeye
