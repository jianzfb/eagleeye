#include "eagleeye/processnode/GOTURNTrackingNode.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/MatrixMath.h"
#include "eagleeye/common/EagleeyeTime.h"
#include "eagleeye/common/EagleeyeRuntime.h"
#include "eagleeye/common/EagleeyeStr.h"
#include <stdio.h>
#include <stdlib.h>

namespace eagleeye{
GOTURNTrackingNode::GOTURNTrackingNode(EagleeyeRuntimeType runtime){
    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(2);
    // 设置输出端口(端口0)及携带数据类型(TargetT)
    this->setOutputPort(new ImageSignal<int>, OUTPUT_PORT_BOX);
    this->setOutputPort(new ImageSignal<float>, OUTPUT_PORT_SCORE);

    // 设置输入端口（拥有2个输入端口）
	this->setNumberOfInputSignals(2);

    // 输出数据
    this->m_last_box = Matrix<int>(1,4);
    this->m_last_box_score = Matrix<float>(1,1);
    // set writable path
    EAGLEEYE_MONITOR_VAR(std::string, setModelPath, getModelPath, "model","","");

    // target model
    std::string target_model = EagleeyeRuntime(runtime).prefix("goturn_target_%s.dlc");
    std::string search_model = EagleeyeRuntime(runtime).prefix("goturn_search_%s.dlc");
    this->m_template_model = new ModelRun(target_model,
                                          EagleeyeRuntime(runtime).device(),
                                          std::vector<std::string>{"input_target"},
                                          std::vector<std::vector<int64_t>>{std::vector<int64_t>{1,195,195,3}},
                                          std::vector<std::string>{"output_node_target_feat"},
                                          std::vector<std::vector<int64_t>>{std::vector<int64_t>{1,5,5,256}});
    
    std::map<std::string, std::string> output_name_map;
    output_name_map["output_node_target_feat"] = "goturn_share_p_npu/AlexNet_depth_npu/target_pool5/MaxPool";
    this->m_template_model->setOutputNameMap(output_name_map);

    std::map<std::string, std::string> output_name_map2;
    output_name_map2["output_node_target_feat"] = "goturn_share_p_npu/AlexNet_depth_npu/target_pool5/MaxPool";
    this->m_template_model->setOutputNameMap2(output_name_map2);

    // search model
    this->m_tracking_model = new ModelRun(search_model,
                                          EagleeyeRuntime(runtime).device(),
                                          std::vector<std::string>{"input_target","input_search"},
                                          std::vector<std::vector<int64_t>>{std::vector<int64_t>{1,5,5,256},std::vector<int64_t>{1,195,195,3}},
                                          std::vector<std::string>{"output_node_cls","output_node_reg"},
                                          std::vector<std::vector<int64_t>>{std::vector<int64_t>{1,1,1,2},std::vector<int64_t>{1,1,1,4}});

    std::map<std::string, std::string> tracking_output_name_map;
    tracking_output_name_map["output_node_cls"] = "goturn_share_p_npu/Softmax";
    tracking_output_name_map["output_node_reg"] = "goturn_share_p_npu/SeparableConv2d_4/separable_conv2d";
    this->m_tracking_model->setOutputNameMap(tracking_output_name_map);

    std::map<std::string, std::string> tracking_output_name_map2;
    tracking_output_name_map2["output_node_cls"] = "goturn_share_p_npu/Softmax";
    tracking_output_name_map2["output_node_reg"] = "goturn_share_p_npu/SeparableConv2d_4/BiasAdd";
    this->m_tracking_model->setOutputNameMap2(tracking_output_name_map2);

    // create hanning window
    this->m_template_size = 195;
    int hann_length = 261;
    int hann_truncate_row_begin = (hann_length - m_template_size)/2;
    int hann_truncate_row_end = hann_truncate_row_begin + m_template_size + 1;
    int hann_truncate_col_begin = (hann_length - m_template_size)/2;
    int hann_truncate_col_end = hann_truncate_row_begin + m_template_size + 1;
    
    Matrix<float> hann_vec = hanning(hann_length);
    Matrix<float> hann_row_truncate = hann_vec(Range(0, 1),Range(hann_truncate_row_begin, hann_truncate_row_end));
    Matrix<float> hann_col_truncate = hann_vec(Range(0, 1),Range(hann_truncate_col_begin, hann_truncate_col_end));
    m_hanning_window = outer(hann_row_truncate,hann_col_truncate);

    this->is_init = false;
}

GOTURNTrackingNode::~GOTURNTrackingNode(){

}

void GOTURNTrackingNode::executeNodeInfo(){
    //get input image signal
	ImageSignal<Array<unsigned char, 3>>* input_img_sig = (ImageSignal<Array<unsigned char, 3>>*)(this->m_input_signals[0]);
    ImageSignal<int>* target_box_sig = (ImageSignal<int>*)(this->m_input_signals[1]);
    ImageSignal<int>* tracking_box_sig = (ImageSignal<int>*)(this->m_output_signals[0]);
	ImageSignal<float>* tracking_score_sig = (ImageSignal<float>*)(this->m_output_signals[1]);
    if (!input_img_sig || !target_box_sig){
		EAGLEEYE_LOGE("input image is not correct...\n");
		return;
	}

    // static int global_frame_index = 0;
    Matrix<Array<unsigned char, 3>> image = input_img_sig->getData();
    Matrix<int> target_box = target_box_sig->getData();

    if(target_box.rows() > 0 && target_box.cols() > 0){
        int x = target_box.at(0,0);
        int y = target_box.at(0,1);
        int w = target_box.at(0,2);
        int h = target_box.at(0,3);

        int rows = image.rows();
        int cols = image.cols();
        int roi[4] = {x,y,w,h};
        
        int context_min_x,context_min_y,context_max_x,context_max_y,left_pad,top_pad;
        Matrix<Array<unsigned char, 3>> template_patch = 
                                    this->getSubWindow(image, 
                                                       roi,
                                                       w,
                                                       h,
                                                       context_min_x,
                                                       context_min_y,
                                                       context_max_x,
                                                       context_max_y,
                                                       top_pad,
                                                       left_pad);
        Matrix<Array<float,3>> template_image_f(template_patch.rows(), template_patch.cols());
        for(int i=0; i<template_patch.rows(); ++i){
            unsigned char* template_image_ptr = (unsigned char*)template_patch.row(i);
            float* template_image_f_ptr = (float*)template_image_f.row(i);
            float* hann_matrix_f_ptr = (float*)m_hanning_window.row(i);
            for(int j=0; j<template_patch.cols(); ++j){
                template_image_f_ptr[j*3] = template_image_ptr[j*3] * hann_matrix_f_ptr[j] / 255.0f;                // r
                template_image_f_ptr[j*3+1] = template_image_ptr[j*3+1] * hann_matrix_f_ptr[j] / 255.0f;            // g
                template_image_f_ptr[j*3+2] = template_image_ptr[j*3+2] * hann_matrix_f_ptr[j] / 255.0f;            // b
            }
        }
        
        // std::string template_image_path = "./template/template_image_"+tos(global_frame_index)+".bin";
        // FILE* template_image_fp = fopen(template_image_path.c_str(),"wb");
        // fwrite(template_image_f.dataptr(), sizeof(float), 1*195*195*3, template_image_fp);
        // fclose(template_image_fp);

        // std::string search_image_path = "./template/search_image_"+tos(global_frame_index)+".bin";
        // FILE* search_image_fp = fopen(search_image_path.c_str(),"wb");
        // fwrite(template_image_f.dataptr(), sizeof(float), 1*195*195*3, search_image_fp);
        // fclose(search_image_fp);

        std::map<std::string, unsigned char*> inputs;
        inputs["input_target"] = (unsigned char*)template_image_f.dataptr();
        std::map<std::string, unsigned char*> outputs;
        EAGLEEYE_TIME_START(templatemodel);
        bool status = this->m_template_model->run(inputs, outputs);
        EAGLEEYE_TIME_END(templatemodel);

        this->m_template_tensor = Tensor<float>(std::vector<int64_t>{1,5,5,256}, outputs["output_node_target_feat"], true);
        
        // std::string template_tensor_path = "./template/template_tensor_"+tos(global_frame_index)+".bin";
        // FILE* template_tensor_fp = fopen(template_tensor_path.c_str(),"wb");
        // fwrite(m_template_tensor.dataptr(), sizeof(float), 1*5*5*256, template_tensor_fp);
        // fclose(template_tensor_fp);
        
        // FILE* fp = fopen("./ss_for_target.txt", "a");
        // std::string mm = "/home/zhangjian8/snpe-workspace/autozoom-goturn/data/";
        // mm = mm + "template_image_"+tos(global_frame_index)+".bin\n";
        // fwrite(mm.c_str(),sizeof(char), mm.length(), fp);
        // fclose(fp);

        // FILE* fp2 = fopen("./ss_for_search.txt", "a");
        // mm = "/home/zhangjian8/snpe-workspace/autozoom-goturn/data/";
        // mm = mm + "search_image_"+tos(global_frame_index)+".bin";
        // mm = mm + " ";
        // mm = mm + "/home/zhangjian8/snpe-workspace/autozoom-goturn/data/";
        // mm = mm + "template_tensor_"+tos(global_frame_index)+".bin\n";
        // fwrite(mm.c_str(),sizeof(char), mm.length(), fp);
        // fclose(fp2);

        // global_frame_index += 1;
        // last box and center
        m_last_box.at(0,0) = x;
        m_last_box.at(0,1) = y;
        m_last_box.at(0,2) = w;
        m_last_box.at(0,3) = h;
        tracking_box_sig->setData(m_last_box);

        m_last_box_score.at(0,0) = 1.0f;
        tracking_score_sig->setData(m_last_box_score);

        EAGLEEYE_LOGD("set tracking box %d %d %d %d",int(x),int(y),int(w),int(h));
        this->m_init_w = w;
        this->m_init_h = h;

        this->is_init = true;
        target_box_sig->makeempty();
        return;
    }

    if(!this->is_init){
        return;
    }

    int last_roi[4] = {int(m_last_box.at(0,0)),
                       int(m_last_box.at(0,1)),
                       int(m_last_box.at(0,2)),
                       int(m_last_box.at(0,3))};
    float scale = 1.0f;
    EAGLEEYE_TIME_START(getsearchpatch);
    int context_min_x,context_min_y,context_max_x,context_max_y,left_pad,top_pad;
    Matrix<Array<unsigned char, 3>> search_image = 
                        this->getSubWindow(image, 
                                           last_roi,
                                           this->m_init_w,
                                           this->m_init_h,
                                           context_min_x,
                                           context_min_y,
                                           context_max_x,
                                           context_max_y,
                                           top_pad,
                                           left_pad);
    EAGLEEYE_TIME_END(getsearchpatch);

    Matrix<Array<float, 3>> search_image_f(search_image.rows(), search_image.cols());
    for(int i=0; i<search_image.rows(); ++i){
        unsigned char* search_image_ptr = (unsigned char*)search_image.row(i);
        float* search_image_f_ptr = (float*)search_image_f.row(i);
        float* hann_matrix_f_ptr = (float*)m_hanning_window.row(i);
        for(int j=0; j<search_image_f.cols(); ++j){
            search_image_f_ptr[j*3] = search_image_ptr[j*3] * hann_matrix_f_ptr[j] / 255.0f;                    // r
            search_image_f_ptr[j*3+1] = search_image_ptr[j*3+1] * hann_matrix_f_ptr[j] / 255.0f;                // g
            search_image_f_ptr[j*3+2] = search_image_ptr[j*3+2] * hann_matrix_f_ptr[j] / 255.0f;                // b
        }
    }

    std::map<std::string, unsigned char*> search_inputs;
    search_inputs["input_target"] = (unsigned char*)this->m_template_tensor.dataptr();
    search_inputs["input_search"] = (unsigned char*)search_image_f.dataptr();
    std::map<std::string, unsigned char*> search_outputs;
    EAGLEEYE_TIME_START(searchmodel);
    bool status = this->m_tracking_model->run(search_inputs, search_outputs);
    EAGLEEYE_TIME_END(searchmodel);

    Tensor<float> cls_output = Tensor<float>(std::vector<int64_t>{1,1,1,2}, search_outputs["output_node_cls"],true);
    float* cls_output_ptr = cls_output.dataptr();
    Tensor<float> reg_output = Tensor<float>(std::vector<int64_t>{1,1,1,4},search_outputs["output_node_reg"],true);
    float* reg_output_ptr = reg_output.dataptr();

    float context_w = context_max_x - context_min_x;
    float context_h = context_max_y - context_min_y;
    float box_min_x = reg_output_ptr[0] / 10.0f * context_w;
    float box_min_y = reg_output_ptr[1] / 10.0f * context_h;
    float box_w = reg_output_ptr[2] / 10.0f * context_w;
    float box_h = reg_output_ptr[3] / 10.0f * context_h;
    
    m_last_box.at(0,0) = int(context_min_x - left_pad) + box_min_x;
    m_last_box.at(0,1) = int(context_min_y - top_pad) + box_min_y;
    m_last_box.at(0,2) = box_w;
    m_last_box.at(0,3) = box_h;
    tracking_box_sig->setData(m_last_box);

    m_last_box_score.at(0,0) = cls_output_ptr[1];
    tracking_score_sig->setData(m_last_box_score);
}   

bool GOTURNTrackingNode::selfcheck(){
    return true;
}

void GOTURNTrackingNode::setModelPath(std::string path){
    this->m_template_model->setWritablePath(path);
    this->m_tracking_model->setWritablePath(path);

    this->m_template_model->initialize();
    this->m_tracking_model->initialize();
}

void GOTURNTrackingNode::getModelPath(std::string& path){
    path = this->m_template_model->getWritablePath();
}

Matrix<Array<unsigned char, 3>> GOTURNTrackingNode::getTemplateImage(Matrix<Array<unsigned char, 3>> frame, float* roi){
    return Matrix<Array<unsigned char, 3>>();
}

Matrix<Array<unsigned char, 3>> GOTURNTrackingNode::getSearchImage(Matrix<Array<unsigned char, 3>> frame, float* roi){
    return Matrix<Array<unsigned char, 3>>();
}

Matrix<Array<unsigned char, 3>> 
GOTURNTrackingNode::getSubWindow(Matrix<Array<unsigned char, 3>> frame, 
                                 int* bbox, 
                                 int init_w,
                                 int init_h,
                                 int& context_xmin, 
                                 int& context_ymin, 
                                 int& context_xmax,
                                 int& context_ymax,
                                 int& top_pad, 
                                 int& left_pad){
    EAGLEEYE_TIME_START(presubwindow);
    int rows = frame.rows();
    int cols = frame.cols();

    // padding
    int box_w = bbox[2];
    int box_h = bbox[3];
    context_xmin = floor(bbox[0] - 0.25*(box_w+init_w));
    context_xmax = context_xmin + box_w + init_w;
    context_ymin = floor(bbox[1] - 0.25*(box_h+init_h));
    context_ymax = context_ymin + box_h + init_h;

    left_pad = int(EAGLEEYE_MAX(0.0f, -context_xmin));
    top_pad = int(EAGLEEYE_MAX(0.0f, -context_ymin));
    int right_pad = int(EAGLEEYE_MAX(0.0f, context_xmax - cols));
    int bottom_pad = int(EAGLEEYE_MAX(0.0f, context_ymax - rows));

    context_xmin = context_xmin + left_pad;
    context_xmax = context_xmax + left_pad;
    context_ymin = context_ymin + top_pad;
    context_ymax = context_ymax + top_pad;

    int context_rows = context_ymax+1-context_ymin;
    int context_cols = context_xmax+1-context_xmin;

    Matrix<Array<unsigned char,3>> local_window(context_rows, context_cols);
    unsigned char* local_window_ptr = (unsigned char*)local_window.dataptr();
    unsigned char* frame_ptr = (unsigned char*)frame.dataptr();
    int context_stride = context_cols*3;
    int stride = frame.cols() * 3;
    if(left_pad > 0 || right_pad > 0 || top_pad > 0 || bottom_pad > 0){
        for(int i=0; i<context_rows; ++i){
            for(int j=0; j<context_cols; ++j){
                int ii = i + context_ymin;
                int jj = j + context_xmin;
                if(ii >= top_pad && jj >= left_pad && ii < top_pad+rows && jj < left_pad+cols){
                    // 
                    local_window_ptr[i*context_stride+j*3] = frame_ptr[(ii-top_pad)*stride + (jj-left_pad)*3];
                    local_window_ptr[i*context_stride+j*3+1] = frame_ptr[(ii-top_pad)*stride + (jj-left_pad)*3+1];
                    local_window_ptr[i*context_stride+j*3+2] = frame_ptr[(ii-top_pad)*stride + (jj-left_pad)*3+2];
                }
            }
        }
    }
    else{
        for(int i=0; i<context_rows; ++i){
            for(int j=0; j<context_cols; ++j){
                int ii = i + context_ymin;
                int jj = j + context_xmin;
                local_window_ptr[i*context_stride+j*3] = frame_ptr[ii*stride + jj*3];
                local_window_ptr[i*context_stride+j*3+1] = frame_ptr[ii*stride + jj*3+1];
                local_window_ptr[i*context_stride+j*3+2] = frame_ptr[ii*stride + jj*3+2];
            }
        }
    }
    EAGLEEYE_TIME_END(presubwindow);

    EAGLEEYE_TIME_START(resize);
    Matrix<Array<unsigned char,3>> resized_window = resize(local_window, 
                                                           195,
                                                           195, 
                                                           BILINEAR_INTERPOLATION);
    EAGLEEYE_TIME_END(resize);
    return resized_window;
}
}