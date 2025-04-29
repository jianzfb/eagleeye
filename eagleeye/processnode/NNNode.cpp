#include "eagleeye/processnode/NNNode.h"
#include "eagleeye/basic/MatrixMath.h"
#include "eagleeye/engine/nano/op/factory.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/AnyMonitor.h"

namespace eagleeye
{
NNNode::NNNode(int thread_num, CPUAffinityPolicy performance){
    m_g = new dataflow::Graph(std::vector<EagleeyeRuntime>(), thread_num, performance);
    m_is_init = false;

	EAGLEEYE_MONITOR_VAR(std::string, setModelFolder, getModelFolder, "model_folder", "", "");
    EAGLEEYE_MONITOR_VAR(std::string, setWritableFolder, getWritableFolder, "writable_path", "", "");
    EAGLEEYE_MONITOR_VAR(std::string, setClear, getClear, "clear", "", "");
    EAGLEEYE_MONITOR_VAR(std::string, setOpValue, getOpValue, "opvalue", "", "");
    EAGLEEYE_MONITOR_VAR(std::string, setOpStrValue, getOpStrValue, "opstrvalue", "", "");
}

NNNode::~NNNode(){
    delete m_g;

    for(int cache_input_i=0; cache_input_i<m_cache_input.size(); ++cache_input_i){
        delete m_cache_input[cache_input_i];
    }
} 

void NNNode::executeNodeInfo(){
    // 1.step 输入绑定
    int signal_num = this->getNumberOfInputSignals();
    if(m_cache_input.size() == 0){
        for(int signal_i=0; signal_i<signal_num; ++signal_i){
            m_cache_input.push_back(
                this->getInputPort(signal_i)->make()
            );
        }
    }

    std::map<std::string, std::tuple<void*, std::vector<int64_t>, EagleeyeType>> graph_input_map;
    for(int sig_i=0; sig_i<signal_num; ++sig_i){
        void* data = NULL;
        size_t* data_size;
        int data_dims = 0;
        int data_type = -1;
        m_cache_input[sig_i]->copy(this->getInputPort(sig_i));
        m_cache_input[sig_i]->getSignalContent(data, data_size, data_dims, data_type);

        std::string sig_i_name = this->m_input_map[sig_i];

        // 允许接受任意格式
        std::vector<int64_t> data_shape(data_dims);
        for(int dim_i=0; dim_i<data_dims; ++dim_i){
            data_shape[dim_i] = data_size[dim_i];
        }
        graph_input_map[sig_i_name] = std::make_tuple(data, data_shape, EagleeyeType(data_type));
    }

    // 2.step 运行网络
    std::map<std::string, std::tuple<void*, std::vector<int64_t>, EagleeyeType>> graph_output_map;
    signal_num = this->getNumberOfOutputSignals();
    for(int sig_i=0; sig_i<signal_num; ++sig_i){
        graph_output_map[m_output_map[sig_i].first] = std::tuple<void*, std::vector<int64_t>, EagleeyeType>(NULL, {}, EAGLEEYE_UNDEFINED);
    }
    m_g->run(graph_input_map, graph_output_map);

    // 3.step 输出绑定
    for(int sig_i=0; sig_i<this->getNumberOfOutputSignals(); ++sig_i){
        std::pair<std::string, int> sig_info = this->m_output_map[sig_i];
        std::tuple<void*, std::vector<int64_t>, EagleeyeType> p;
        m_g->get(sig_info.first, sig_info.second, p);

        SignalType tt = this->getOutputPort(sig_i)->getSignalType();
        if(tt == EAGLEEYE_SIGNAL_IMAGE || tt == EAGLEEYE_SIGNAL_RGB_IMAGE || tt == EAGLEEYE_SIGNAL_BGR_IMAGE){
            // 需要保证 NNGraph输出 HxWx3
            ImageSignal<Array<unsigned char, 3>>* image_sig = (ImageSignal<Array<unsigned char, 3>>*)(this->getOutputPort(sig_i));
            Matrix<Array<unsigned char, 3>> image = image_sig->getData();
            if(image.rows() != std::get<1>(p)[0]|| image.cols() != std::get<1>(p)[1]){
                image = Matrix<Array<unsigned char, 3>>(std::get<1>(p)[0], std::get<1>(p)[1]);
            }

            memcpy(image.dataptr(), std::get<0>(p), sizeof(unsigned char)*std::get<1>(p)[0]*std::get<1>(p)[1]*3);
            image_sig->setData(image);
        }
        else if(tt == EAGLEEYE_SIGNAL_RGBA_IMAGE || tt == EAGLEEYE_SIGNAL_BGRA_IMAGE){
            // 需要保证 NNGraph输出 HxWx4
            ImageSignal<Array<unsigned char, 4>>* image_sig = (ImageSignal<Array<unsigned char, 4>>*)(this->getOutputPort(sig_i));
            Matrix<Array<unsigned char, 4>> image = image_sig->getData();

            if(image.rows() != std::get<1>(p)[0] || image.cols() != std::get<1>(p)[1]){
                image = Matrix<Array<unsigned char, 4>>(std::get<1>(p)[0], std::get<1>(p)[1]);
            }

            memcpy(image.dataptr(), std::get<0>(p), sizeof(unsigned char)*std::get<1>(p)[0]*std::get<1>(p)[1]*4);
            image_sig->setData(image);
        }
        else if(tt == EAGLEEYE_SIGNAL_GRAY_IMAGE || tt == EAGLEEYE_SIGNAL_MASK || tt == EAGLEEYE_SIGNAL_UCMATRIX){
            // 需要保证 NNGraph输出 HxW
            ImageSignal<unsigned char>* image_sig = (ImageSignal<unsigned char>*)(this->getOutputPort(sig_i));
            Matrix<unsigned char> image = image_sig->getData();

            if(image.rows() != std::get<1>(p)[0] || image.cols() != std::get<1>(p)[1]){
                image = Matrix<unsigned char>(std::get<1>(p)[0], std::get<1>(p)[1]);
            } 

            memcpy(image.dataptr(),std::get<0>(p), sizeof(unsigned char)*std::get<1>(p)[0]*std::get<1>(p)[1]);
            image_sig->setData(image);
        }
        else if(tt == EAGLEEYE_SIGNAL_SWITCH){
            // 需要保证 输出 1
            BooleanSignal* boolean_sig = (BooleanSignal*)(this->getOutputPort(sig_i));
            int* data = ((int*)std::get<0>(p));
            bool is_true = (*data) > 0 ? true : false;
            boolean_sig->setData(is_true);
        }
        else if(tt == EAGLEEYE_SIGNAL_CLS || tt == EAGLEEYE_SIGNAL_STATE || tt == EAGLEEYE_SIGNAL_IMATRIX){
            ImageSignal<int>* image_sig = (ImageSignal<int>*)(this->getOutputPort(sig_i));
            Matrix<int> image = image_sig->getData();
            if(image.rows() != std::get<1>(p)[0] || image.cols() != std::get<1>(p)[1]){
                image = Matrix<int>(std::get<1>(p)[0], std::get<1>(p)[1]);
            } 

            memcpy(image.dataptr(),std::get<0>(p), sizeof(int)*std::get<1>(p)[0]*std::get<1>(p)[1]);
            image_sig->setData(image);
        }
        else if(tt == EAGLEEYE_SIGNAL_DET || tt == EAGLEEYE_SIGNAL_DET_EXT || tt == EAGLEEYE_SIGNAL_POS_2D || tt == EAGLEEYE_SIGNAL_POS_3D || tt == EAGLEEYE_SIGNAL_FMATRIX){
            ImageSignal<float>* image_sig = (ImageSignal<float>*)(this->getOutputPort(sig_i));
            Matrix<float> image = image_sig->getData();
            if(image.rows() != std::get<1>(p)[0] || image.cols() != std::get<1>(p)[1]){
                image = Matrix<float>(std::get<1>(p)[0], std::get<1>(p)[1]);
            } 

            memcpy(image.dataptr(), std::get<0>(p), sizeof(float)*std::get<1>(p)[0]*std::get<1>(p)[1]);
            image_sig->setData(image);
        }
        else if(tt == EAGLEEYE_SIGNAL_RECT || tt == EAGLEEYE_SIGNAL_LINE || tt == EAGLEEYE_SIGNAL_POINT){
            ImageSignal<float>* image_sig = (ImageSignal<float>*)(this->getOutputPort(sig_i));
            Matrix<float> image = image_sig->getData();
            if(image.rows() != std::get<1>(p)[0] || image.cols() != std::get<1>(p)[1]){
                image = Matrix<float>(std::get<1>(p)[0], std::get<1>(p)[1]);
            } 

            memcpy(image.dataptr(), std::get<0>(p), sizeof(float)*std::get<1>(p)[0]*std::get<1>(p)[1]);
            image_sig->setData(image);
        }
        else if(tt == EAGLEEYE_SIGNAL_TIMESTAMP){
            ImageSignal<double>* image_sig = (ImageSignal<double>*)(this->getOutputPort(sig_i));
            Matrix<double> image = image_sig->getData();
            if(image.rows() != 1 || image.cols() != 1){
                image = Matrix<double>(1, 1);
            }

            if(std::get<1>(p)[0]*std::get<1>(p)[1] > 0){
                memcpy(image.dataptr(), std::get<0>(p), sizeof(double)*std::get<1>(p)[0]*std::get<1>(p)[1]);
            }
            image_sig->setData(image);
        }
        else{
            // Tensor
            TensorSignal* tensor_sig = (TensorSignal*)(this->getOutputPort(sig_i));
            tensor_sig->setData(Tensor(std::get<1>(p), std::get<2>(p), DataFormat::AUTO, std::get<0>(p), true));
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
                m_output_map[to_node_port] = std::pair<std::string, int>(node_name,0);
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

    this->m_is_init = true;
    return true;
}

int NNNode::getOpGraphIn(){
    return this->m_g->getEntryNodes().size();
}

void NNNode::getOpGraphOutInfo(int i, std::pair<std::string, int>& info){
    if(m_output_map.find(i) == m_output_map.end()){
        return;
    }

    info = m_output_map[i];
}
void NNNode::getOpGraphInInfo(int i, std::string& info){
    if(m_input_map.find(i) == m_input_map.end()){
        return;
    }

    info = m_input_map[i];
}

void NNNode::analyze(std::vector<std::string> in_ops, std::vector<std::pair<std::string, int>> out_ops){
    // 创建输入信号
    int input_signal_num = in_ops.size();
    this->setNumberOfInputSignals(input_signal_num);
    for(int i=0; i<input_signal_num; ++i){
        this->m_input_map[i] = in_ops[i];
    }

    int output_signal_num = out_ops.size();
    // 创建输出信号
    this->setNumberOfOutputSignals(output_signal_num);
    for(int i=0; i<output_signal_num; ++i){
        this->m_output_map[i] = out_ops[i];
    }
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
    else if(type_str == "EAGLEEYE_SIGNAL_TENSOR"){
        this->setOutputPort(new TensorSignal(), port);
        this->getOutputPort(port)->setSignalType(EAGLEEYE_SIGNAL_TENSOR);
    }
    else if(type_str == "EAGLEEYE_SIGNAL_TIMESTAMP"){
        this->setOutputPort(new ImageSignal<double>(), port);
        this->getOutputPort(port)->setSignalType(EAGLEEYE_SIGNAL_TIMESTAMP);
    }
    else{
        EAGLEEYE_LOGE("Dont support signal type %s.", type_str.c_str());
    }
}

void NNNode::setModelFolder(const std::string model_folder){
    std::map<std::string, std::vector<std::string>> info;
    info["model_folder"].push_back(model_folder);
    std::vector<dataflow::Node*> nodes = this->m_g->getNodes();
    for(int i=0; i<nodes.size(); ++i){
        nodes[i]->init(info);
    }
}
void NNNode::getModelFolder(std::string& model_folder){
    // do nothing
}

void NNNode::setWritableFolder(const std::string writable_folder){
    std::map<std::string, std::vector<std::string>> info;
    info["writable_path"].push_back(writable_folder);
    std::vector<dataflow::Node*> nodes = this->m_g->getNodes();
    for(int i=0; i<nodes.size(); ++i){
        nodes[i]->init(info);
    }
}
void NNNode::getWritableFolder(std::string& writable_folder){
    // do nothing
}

void NNNode:: setClear(const std::string name){
    std::vector<dataflow::Node*> nodes = this->m_g->getNodes();
    for(int i=0; i<nodes.size(); ++i){
        if(nodes[i]->name == name){
            nodes[i]->clear();
        }
    }
}
void NNNode::getClear(std::string& name){
    // do nothing
}

void NNNode::setOpValue(const std::string name){
    // name format: node_name:var_name:var_value
    std::string separator = ":";
    std::vector<std::string> name_and_value = split(name, separator);
    if(name_and_value.size() != 3){
        EAGLEEYE_LOGE("value %s format error(expected format node_name:var_name:var_value).", name.c_str());
        return;
    }

    std::string node_name = name_and_value[0];
    std::string var_name = name_and_value[1];
    float var_value = tof<float>(name_and_value[2]);
    std::vector<dataflow::Node*> nodes = this->m_g->getNodes();
    for(int i=0; i<nodes.size(); ++i){
        if(nodes[i]->name == node_name){
            nodes[i]->init(std::map<std::string, std::vector<float>>({{var_name,{var_value}}}));
        }
    }
}

void NNNode::getOpValue(std::string& name){
    // do nothing
}

void NNNode::setOpStrValue(const std::string name){
    // name format: node_name:var_name:var_value
    std::string separator = ":";
    std::vector<std::string> name_and_value = split(name, separator);
    if(name_and_value.size() != 3){
        EAGLEEYE_LOGE("value %s format error(expected format node_name:var_name:var_value).", name.c_str());
        return;
    }

    std::string node_name = name_and_value[0];
    std::string var_name = name_and_value[1];
    std::string var_value = name_and_value[2];
    std::vector<dataflow::Node*> nodes = this->m_g->getNodes();
    for(int i=0; i<nodes.size(); ++i){
        if(nodes[i]->name == node_name){
            nodes[i]->init(std::map<std::string, std::vector<std::string>>({{var_name,{var_value}}}));
        }
    }
}

void NNNode::getOpStrValue(std::string& name){
    // do nothing
}

} // namespace eagleeye
