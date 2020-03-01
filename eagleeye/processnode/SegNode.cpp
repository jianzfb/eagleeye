#include "eagleeye/processnode/SegNode.h"
#include "eagleeye/basic/MatrixMath.h"
#include "eagleeye/common/EagleeyeTime.h"

namespace eagleeye{
SegNode::SegNode(std::string model_name, 
                 std::string device, 
                 std::string input_node, 
                 std::vector<int> input_size,
                 std::string output_node,
                 std::vector<int> output_size,
                 std::vector<std::string> snpe_special_nodes,
                 SegResizeMode resized_mode){
    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
    // 设置输出端口(端口0)及携带数据类型(TargetT)
    this->setOutputPort(new ImageSignal<float>, OUTPUT_PORT_MASK);

    // 设置输入端口（拥有1个输入端口）
	this->setNumberOfInputSignals(1);

    // set writable path
    EAGLEEYE_MONITOR_VAR(std::string, setModelPath, getModelPath, "modelpath","","");

    this->m_model_h = 512;
    this->m_model_w = 512;
    this->m_mean_r = 128;
    this->m_mean_g = 128;
    this->m_mean_b = 128;

    this->m_var_r = 255.0f;
    this->m_var_g = 255.0f;
    this->m_var_b = 255.0f;

    this->m_model_h = input_size[1];
    this->m_model_w = input_size[2];

    this->m_output_h = output_size[1];
    this->m_output_w = output_size[2];
    this->m_class_num = output_size[3];

    this->m_resized_mode = resized_mode;

    // target model
    this->m_seg_model = new ModelRun(model_name,
                                          device,
                                          std::vector<std::string>{input_node},
                                          std::vector<std::vector<int64_t>>{std::vector<int64_t>{1,m_model_h,m_model_w,3}},
                                          std::vector<std::string>{output_node},
                                          std::vector<std::vector<int64_t>>{std::vector<int64_t>{1,m_output_h,m_output_w,m_class_num}});
    
    std::map<std::string, std::string> output_name_map;
    output_name_map[output_node] = snpe_special_nodes[0];
    this->m_seg_model->setOutputNameMap(output_name_map);

    std::map<std::string, std::string> output_name_map2;
    output_name_map2[output_node] = snpe_special_nodes[1];
    this->m_seg_model->setOutputNameMap2(output_name_map2);

    this->m_input_node = input_node;
    this->m_output_node = output_node;
    m_model_input_f = Matrix<Array<float,3>>(this->m_model_h, this->m_model_w);
}   

SegNode::~SegNode(){
    delete m_seg_model;
} 

Tensor<float> SegNode::runSeg(Matrix<Array<unsigned char,3>> frame){
    // resize to standard size
    EAGLEEYE_TIME_START(preprocess);
    
    if(m_resized_mode == DEFAULT_RESIZE){
    EAGLEEYE_LOGD("use default resize mode");
        Matrix<Array<unsigned char,3>> standard_img = resize(frame, this->m_model_h, this->m_model_w, BILINEAR_INTERPOLATION);
        for(int r=0; r<this->m_model_h; ++r){
            float* model_input_f_ptr = (float*)m_model_input_f.row(r);
            unsigned char* standard_img_ptr = (unsigned char*)standard_img.row(r);
            for(int c=0; c<this->m_model_w; ++c){
                model_input_f_ptr[c*3] = ((float)(standard_img_ptr[c*3]) - m_mean_r)/m_var_r;
                model_input_f_ptr[c*3+1] = ((float)(standard_img_ptr[c*3+1]) - m_mean_g)/m_var_g;
                model_input_f_ptr[c*3+2] = ((float)(standard_img_ptr[c*3+2]) - m_mean_b)/m_var_b;
            }
        }
    }
    else{
        EAGLEEYE_LOGD("use same scale resize mode");
        int new_height = 0, new_width = 0;
        float ratio_rows_cols = 1.0f * frame.cols() / frame.rows();
        if (ratio_rows_cols > 1.0f) {
            new_width = this->m_model_w;
            new_height = this->m_model_h / ratio_rows_cols;
        } else {
            new_width = this->m_model_w * ratio_rows_cols;
            new_height = this->m_model_h;
        }
        EAGLEEYE_TIME_START(preprocess_resize);
        Matrix<Array<unsigned char, 3>> tmp = resize(frame, new_height, new_width, BILINEAR_INTERPOLATION);
        EAGLEEYE_TIME_END(preprocess_resize);

        int start_r = (this->m_model_h - new_height) / 2;
        int end_r = start_r + new_height;
        int start_c = (this->m_model_w - new_width) / 2;
        int end_c = start_c + new_width;

        EAGLEEYE_TIME_START(preprocess_subdiv);      
        for(int r=start_r; r<end_r; ++r){
            float* model_input_f_ptr = (float*)m_model_input_f.row(r);
            unsigned char* tmp_ptr = (unsigned char*)tmp.row(r-start_r);
            for(int c=start_c; c<end_c; ++c){
                int index = c-start_c;
                model_input_f_ptr[c*3] = ((float)(tmp_ptr[index*3]) - m_mean_r)/m_var_r;
                model_input_f_ptr[c*3+1] = ((float)(tmp_ptr[index*3+1]) - m_mean_g)/m_var_g;
                model_input_f_ptr[c*3+2] = ((float)(tmp_ptr[index*3+2]) - m_mean_b)/m_var_b;
            }
        }
        EAGLEEYE_TIME_END(preprocess_subdiv);
    }
    EAGLEEYE_TIME_END(preprocess);

    // run model
    std::map<std::string, unsigned char*> inputs;
    inputs[this->m_input_node] = (unsigned char*)m_model_input_f.dataptr();
    std::map<std::string, unsigned char*> outputs;
    EAGLEEYE_TIME_START(segmodel);
    bool status = this->m_seg_model->run(inputs, outputs);
    EAGLEEYE_TIME_END(segmodel);
    Tensor<float> output_tensor= Tensor<float>(std::vector<int64_t>{1,this->m_output_h,this->m_output_w,this->m_class_num}, EagleeyeRuntime(EAGLEEYE_CPU), outputs[this->m_output_node]);
    return output_tensor;
}

void SegNode::setModelPath(std::string path){
    this->m_seg_model->setWritablePath(path);
    this->m_seg_model->initialize();
}

void SegNode::getModelPath(std::string& path){
    path = this->m_seg_model->getWritablePath();
}

bool SegNode::selfcheck(){
    return true;
}
}