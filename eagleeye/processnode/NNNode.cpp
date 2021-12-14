#include "eagleeye/processnode/NNNode.h"
#include "eagleeye/basic/MatrixMath.h"
#include "eagleeye/engine/nano/op/factory.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"


namespace eagleeye
{
NNNode::NNNode(){
    m_g = new dataflow::Graph(std::vector<EagleeyeRuntime>(), 2);
}   
NNNode::~NNNode(){
    delete m_g;
} 

void NNNode::executeNodeInfo(){
    // 1.step 输入绑定
    int signal_num = this->getNumberOfInputSignals();

    std::map<std::string, std::pair<void*, std::vector<int64_t>>> graph_input_map;
    for(int sig_i=0; sig_i<signal_num; ++sig_i){
        void* data = NULL;
        int data_size[4];
        int data_dims = 0;
        int data_type = -1;
        this->getInputPort(sig_i)->getSignalContent(data, data_size, data_dims, data_type);

        std::string sig_i_name = this->m_input_map[sig_i];
        // 来自NN的输输入，只有NHWC、NC两种格式
        if(this->getInputPort(sig_i)->getSignalType() == EAGLEEYE_SIGNAL_IMAGE || 
            this->getInputPort(sig_i)->getSignalType() == EAGLEEYE_SIGNAL_RGB_IMAGE || 
            this->getInputPort(sig_i)->getSignalType() == EAGLEEYE_SIGNAL_BGR_IMAGE){
            std::vector<int64_t> data_shape(4);
            data_shape[0] = 1; data_shape[1] = data_size[0]; data_shape[2] = data_size[1]; data_shape[3] = 3;
            // NHWC(1,H,W,3)
            graph_input_map[sig_i_name] = std::make_pair(data, data_shape);
        }
        else if(this->getInputPort(sig_i)->getSignalType() == EAGLEEYE_SIGNAL_RGBA_IMAGE ||
                this->getInputPort(sig_i)->getSignalType() == EAGLEEYE_SIGNAL_BGRA_IMAGE){
            std::vector<int64_t> data_shape(4);
            data_shape[0] = 1; data_shape[1] = data_size[0]; data_shape[2] = data_size[1]; data_shape[3] = 4;
            // NHWC(1,H,W,4)
            graph_input_map[sig_i_name] = std::make_pair(data, data_shape);
        }
        else if(this->getInputPort(sig_i)->getSignalType() == EAGLEEYE_SIGNAL_GRAY_IMAGE||
                this->getInputPort(sig_i)->getSignalType() == EAGLEEYE_SIGNAL_MASK){
            std::vector<int64_t> data_shape(4);
            data_shape[0] = 1; data_shape[1] = data_size[0]; data_shape[2] = data_size[1]; data_shape[3] = 1;
            // NHWC(1,H,W,1)
            graph_input_map[sig_i_name] = std::make_pair(data, data_shape);
        }
        else if(this->getInputPort(sig_i)->getSignalType() == EAGLEEYE_SIGNAL_DET ||
                this->getInputPort(sig_i)->getSignalType() == EAGLEEYE_SIGNAL_DET_EXT ||
                this->getInputPort(sig_i)->getSignalType() == EAGLEEYE_SIGNAL_TRACKING ||
                this->getInputPort(sig_i)->getSignalType() == EAGLEEYE_SIGNAL_POS_2D ||
                this->getInputPort(sig_i)->getSignalType() == EAGLEEYE_SIGNAL_POS_3D ||
                this->getInputPort(sig_i)->getSignalType() == EAGLEEYE_SIGNAL_LANDMARK){
            std::vector<int64_t> data_shape(2);
            data_shape[0] = data_size[0]; data_shape[1] = data_size[1];
            // NC(N,C)
            // 网络中需要对不同信号对应的数据进行重排
            graph_input_map[sig_i_name] = std::make_pair(data, data_shape);
        }
        else if(this->getInputPort(sig_i)->getSignalType() == EAGLEEYE_SIGNAL_CLS ||
                this->getInputPort(sig_i)->getSignalType() == EAGLEEYE_SIGNAL_STATE){
            std::vector<int64_t> data_shape(2);
            data_shape[0] = data_size[0]; data_shape[1] = data_size[1];
            // NC(N,C)
            graph_input_map[sig_i_name] = std::make_pair(data, data_shape);    
        }
        else if(this->getInputPort(sig_i)->getSignalType() == EAGLEEYE_SIGNAL_RECT ||
                this->getInputPort(sig_i)->getSignalType() == EAGLEEYE_SIGNAL_LINE || 
                this->getInputPort(sig_i)->getSignalType() == EAGLEEYE_SIGNAL_POINT){
            std::vector<int64_t> data_shape(2);
            data_shape[0] = data_size[0]; data_shape[1] = data_size[1];
            // NC(N,C)
            graph_input_map[sig_i_name] = std::make_pair(data, data_shape);
        }
        else{
            EAGLEEYE_LOGE("Dont support input signal.");
            return;
        }
    }

    // 2.step 运行网络
    std::map<std::string, std::pair<void*, std::vector<int64_t>>> graph_output_map;
    signal_num = this->getNumberOfOutputSignals();
    for(int sig_i=0; sig_i<signal_num; ++sig_i){
        graph_output_map[m_output_map[sig_i]] = std::pair<void*, std::vector<int64_t>>(NULL, {});
    }
    m_g->run(graph_input_map, graph_output_map);

    // 3.step 输出绑定
    for(int sig_i=0; sig_i<signal_num; ++sig_i){
        std::string sig_i_name = this->m_output_map[sig_i];
        std::pair<void*, std::vector<int64_t>> p = graph_output_map[sig_i_name];
        // 来自NN的输出，只有NHWC、NC两种格式
        SignalType tt = this->getOutputPort(sig_i)->getSignalType();
        if(tt == EAGLEEYE_SIGNAL_IMAGE || tt == EAGLEEYE_SIGNAL_RGB_IMAGE || tt == EAGLEEYE_SIGNAL_BGR_IMAGE){
            // NHWC(1,H,W,3)
            ImageSignal<Array<unsigned char, 3>>* image_sig = (ImageSignal<Array<unsigned char, 3>>*)(this->getOutputPort(sig_i));
            Matrix<Array<unsigned char, 3>> image = image_sig->getData();
            
            if(image.rows() != p.second[1] || image.cols() != p.second[2]){
                image = Matrix<Array<unsigned char, 3>>(p.second[1], p.second[2]);
                memcpy(image.dataptr(), p.first, sizeof(unsigned char)*p.second[1]*p.second[2]*3);
            }

            image_sig->setData(image);
        }
        else if(tt == EAGLEEYE_SIGNAL_RGBA_IMAGE || tt == EAGLEEYE_SIGNAL_BGRA_IMAGE){
            // NHWC(1,H,W,4)
            ImageSignal<Array<unsigned char, 4>>* image_sig = (ImageSignal<Array<unsigned char, 4>>*)(this->getOutputPort(sig_i));
            Matrix<Array<unsigned char, 4>> image = image_sig->getData();
            
            if(image.rows() != p.second[1] || image.cols() != p.second[2]){
                image = Matrix<Array<unsigned char, 4>>(p.second[1], p.second[2]);
                memcpy(image.dataptr(), p.first, sizeof(unsigned char)*p.second[1]*p.second[2]*4);
            }

            image_sig->setData(image);
        }
        else if(tt == EAGLEEYE_SIGNAL_GRAY_IMAGE || tt == EAGLEEYE_SIGNAL_MASK){
            // NHWC(1,H,W,1)
            ImageSignal<unsigned char>* image_sig = (ImageSignal<unsigned char>*)(this->getOutputPort(sig_i));
            Matrix<unsigned char> image = image_sig->getData();
            
            if(image.rows() != p.second[1] || image.cols() != p.second[2]){
                image = Matrix<unsigned char>(p.second[1], p.second[2]);
                memcpy(image.dataptr(), p.first, sizeof(unsigned char)*p.second[1]*p.second[2]);
            } 

            image_sig->setData(image);
        }
        else if(tt == EAGLEEYE_SIGNAL_DET || tt == EAGLEEYE_SIGNAL_DET_EXT || tt == EAGLEEYE_SIGNAL_POS_2D || tt == EAGLEEYE_SIGNAL_POS_3D){
            // NC(N,C)
            ImageSignal<float>* image_sig = (ImageSignal<float>*)(this->getOutputPort(sig_i));
            Matrix<float> image = image_sig->getData();
            if(image.rows() != p.second[0] || image.cols() != p.second[1]){
                image = Matrix<float>(p.second[0], p.second[1]);
                memcpy(image.dataptr(), p.first, sizeof(float)*p.second[0]*p.second[1]);
            } 

            image_sig->setData(image);
        }
        else if(tt == EAGLEEYE_SIGNAL_CLS || tt == EAGLEEYE_SIGNAL_STATE){
            // NC(N,C)
            ImageSignal<int>* image_sig = (ImageSignal<int>*)(this->getOutputPort(sig_i));
            Matrix<int> image = image_sig->getData();
            if(image.rows() != p.second[0] || image.cols() != p.second[1]){
                image = Matrix<int>(p.second[0], p.second[1]);
                memcpy(image.dataptr(), p.first, sizeof(int)*p.second[0]*p.second[1]);
            } 

            image_sig->setData(image);
        }
        else if(tt == EAGLEEYE_SIGNAL_RECT || tt == EAGLEEYE_SIGNAL_LINE || tt == EAGLEEYE_SIGNAL_POINT){
            // NC(N,C)
            ImageSignal<float>* image_sig = (ImageSignal<float>*)(this->getOutputPort(sig_i));
            Matrix<float> image = image_sig->getData();
            if(image.rows() != p.second[0] || image.cols() != p.second[1]){
                image = Matrix<float>(p.second[0], p.second[1]);
                memcpy(image.dataptr(), p.first, sizeof(float)*p.second[0]*p.second[1]);
            } 

            image_sig->setData(image);
        }
        else if(tt == EAGLEEYE_SIGNAL_SWITCH){
            // NC(1,1)
            BooleanSignal* boolean_sig = (BooleanSignal*)(this->getOutputPort(sig_i));
            int* data = ((int*)p.first);
            bool is_true = (*data) > 0 ? true : false;
            boolean_sig->setData(is_true);
        }
        else{
            EAGLEEYE_LOGE("Dont support output signal.");
        }
    }
}

bool NNNode::load(neb::CJsonObject nn_config, OpBFuncType op_builder, std::string resource_folder){
    // node: [],
    // attribute: [],
    // topology: []
    // 1.step 获得神经网络构建的节点
    neb::CJsonObject nodes_obj;
    nn_config.Get("node", nodes_obj);
    std::vector<std::string> nodes;
    for(int node_i=0; node_i<nodes_obj.GetArraySize(); ++node_i){
        std::string node_name;
        nodes_obj.Get(node_i, node_name);
        nodes.push_back(node_name);
        EAGLEEYE_LOGD("NN (node %s).", node_name.c_str());
    }

    // 2.step 创建节点算子
    std::map<std::string, dataflow::Node*> nodes_map;
    neb::CJsonObject nodes_attr;
    nn_config.Get("attribute", nodes_attr);
    for(int node_i=0; node_i<nodes.size(); ++node_i){
        if(!nodes_attr.GetKey(nodes[node_i])){
            EAGLEEYE_LOGD("NN (node %s) attr dont exist.", nodes[node_i].c_str());
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

        neb::CJsonObject op_attr;
        node_attr.Get("param", op_attr);
        dataflow::Node* node_ptr = build_node_op(m_g, nodes[node_i], class_name, op_attr);
        if(node_ptr == NULL){
            if(!op_builder(m_g, nodes[node_i], class_name, op_attr, node_ptr, resource_folder)){
                EAGLEEYE_LOGD("NN (node %s class %s) dont support by opbuilder.", nodes[node_i].c_str(), class_name.c_str());
            }
        }

        if(node_ptr == NULL){
            EAGLEEYE_LOGD("NN (node %s class %s) dont support.", nodes[node_i].c_str(), class_name.c_str());
            return false;
        }
        // 记录
        nodes_map[nodes[node_i]] = node_ptr;
    }
    
    // 3.step 构建神经网络拓扑结构
    neb::CJsonObject topology;
    nn_config.Get("topology", topology);

    // 输入配置
    neb::CJsonObject input_config;
    topology.Get("input", input_config);
    for(int port_i=0; port_i<input_config.GetArraySize(); ++port_i){
        neb::CJsonObject port_c;
        input_config.Get(port_i, port_c);

        int from_node_port;
        neb::CJsonObject from_c;
        port_c.Get("from", from_c);
        from_c.Get("port", from_node_port);

        std::string to_node_name;
        int to_node_port = 0;
        neb::CJsonObject to_c;
        port_c.Get("to", to_c);
        to_c.Get("node", to_node_name);
        to_c.Get("port", to_node_port);

        m_input_map[from_node_port] = to_node_name;
    }

    // 其他配置
    for(int node_i=0; node_i<nodes.size(); ++node_i){
        std::string node_name = nodes[node_i];

        if(!topology.KeyExist(node_name)){
            EAGLEEYE_LOGD("NN (node %s) not in topology config.", node_name.c_str());
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

            if(startswith(to_node_name, "output")){
                // 记录外层输出与内层的关联
                m_output_map[to_node_port] = node_name;
            }
            else{
                // 创建算子间的链接关系                
                m_g->bind(nodes_map[node_name], from_node_port, nodes_map[to_node_name], to_node_port);
            }
        }
    }

    // 初始化nn graph
    this->m_g->init(NULL);

    // 输入端口数
    int input_signal_num = m_input_map.size();
    // 输出端口数
    int output_signal_num = m_output_map.size();
    
    // 创建输入信号
    this->setNumberOfInputSignals(input_signal_num);

    // 创建输出信号
    this->setNumberOfOutputSignals(output_signal_num);
    return true;
}

void NNNode::makeOutputSignal(int port,  std::string type_str){
    if(type_str == "EAGLEEYE_SIGNAL_IMAGE" ||
        type_str == "EAGLEEYE_SIGNAL_RGB_IMAGE" ||
        type_str == "EAGLEEYE_SIGNAL_BGR_IMAGE"){
        this->setOutputPort(new ImageSignal<Array<unsigned char, 3>>(), port);
        if(type_str == "EAGLEEYE_SIGNAL_IMAGE"){
            this->getOutputPort(port)->setSignalType(EAGLEEYE_SIGNAL_IMAGE);
        }
        else if(type_str == "EAGLEEYE_SIGNAL_RGB_IMAGE"){
            this->getOutputPort(port)->setSignalType(EAGLEEYE_SIGNAL_RGB_IMAGE);
        }
        else{
            this->getOutputPort(port)->setSignalType(EAGLEEYE_SIGNAL_BGR_IMAGE);
        }
    }
    else if(type_str == "EAGLEEYE_SIGNAL_RGBA_IMAGE" ||
            type_str == "EAGLEEYE_SIGNAL_BGRA_IMAGE"){
        this->setOutputPort(new ImageSignal<Array<unsigned char, 4>>(), port);
        if(type_str == "EAGLEEYE_SIGNAL_RGBA_IMAGE"){
            this->getOutputPort(port)->setSignalType(EAGLEEYE_SIGNAL_RGBA_IMAGE);
        }
        else{
            this->getOutputPort(port)->setSignalType(EAGLEEYE_SIGNAL_BGRA_IMAGE);
        }
    }
    else if(type_str == "EAGLEEYE_SIGNAL_GRAY_IMAGE" ||
            type_str == "EAGLEEYE_SIGNAL_MASK"){
        this->setOutputPort(new ImageSignal<unsigned char>(), port);
        if(type_str == "EAGLEEYE_SIGNAL_GRAY_IMAGE"){
            this->getOutputPort(port)->setSignalType(EAGLEEYE_SIGNAL_GRAY_IMAGE);
        }
        else{
            this->getOutputPort(port)->setSignalType(EAGLEEYE_SIGNAL_MASK);
        }
    }
    else if(type_str == "EAGLEEYE_SIGNAL_DET" ||
            type_str == "EAGLEEYE_SIGNAL_DET_EXT" ||
            type_str == "EAGLEEYE_SIGNAL_TRACKING" ||
            type_str == "EAGLEEYE_SIGNAL_POS_2D" ||
            type_str == "EAGLEEYE_SIGNAL_POS_3D" ||
            type_str == "EAGLEEYE_SIGNAL_LANDMARK"){
        this->setOutputPort(new ImageSignal<float>(), port);
        if(type_str == "EAGLEEYE_SIGNAL_DET"){
            this->getOutputPort(port)->setSignalType(EAGLEEYE_SIGNAL_DET);
        }
        else if(type_str == "EAGLEEYE_SIGNAL_DET_EXT"){
            this->getOutputPort(port)->setSignalType(EAGLEEYE_SIGNAL_DET_EXT);
        }
        else if(type_str == "EAGLEEYE_SIGNAL_TRACKING"){
            this->getOutputPort(port)->setSignalType(EAGLEEYE_SIGNAL_TRACKING);
        }
        else if(type_str == "EAGLEEYE_SIGNAL_POS_2D"){
            this->getOutputPort(port)->setSignalType(EAGLEEYE_SIGNAL_POS_2D);
        }
        else if(type_str == "EAGLEEYE_SIGNAL_POS_3D"){
            this->getOutputPort(port)->setSignalType(EAGLEEYE_SIGNAL_POS_3D);
        }
        else{
            this->getOutputPort(port)->setSignalType(EAGLEEYE_SIGNAL_LANDMARK);
        }
    }
    else if(type_str == "EAGLEEYE_SIGNAL_CLS" ||
            type_str == "EAGLEEYE_SIGNAL_STATE"){
        this->setOutputPort(new ImageSignal<int>(), port);
        if(type_str == "EAGLEEYE_SIGNAL_CLS"){
            this->getOutputPort(port)->setSignalType(EAGLEEYE_SIGNAL_CLS);
        }
        else{
            this->getOutputPort(port)->setSignalType(EAGLEEYE_SIGNAL_STATE);
        }
    }
    else if(type_str == "EAGLEEYE_SIGNAL_SWITCH"){
        this->setOutputPort(new BooleanSignal(), port);
        this->getOutputPort(port)->setSignalType(EAGLEEYE_SIGNAL_SWITCH);
    }
    else if(type_str == "EAGLEEYE_SIGNAL_RECT" ||
            type_str == "EAGLEEYE_SIGNAL_LINE" ||
            type_str == "EAGLEEYE_SIGNAL_POINT"){
        this->setOutputPort(new ImageSignal<float>(), port);
        if(type_str == "EAGLEEYE_SIGNAL_RECT"){
            this->getOutputPort(port)->setSignalType(EAGLEEYE_SIGNAL_RECT);
        }
        else if(type_str == "EAGLEEYE_SIGNAL_LINE"){
            this->getOutputPort(port)->setSignalType(EAGLEEYE_SIGNAL_LINE);
        }
        else{
            this->getOutputPort(port)->setSignalType(EAGLEEYE_SIGNAL_POINT);
        }
    }
    else{
        EAGLEEYE_LOGE("Dont support signal type %s.", type_str.c_str());
    }
}
} // namespace eagleeye
