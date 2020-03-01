#include "eagleeye/processnode/SimRPNTrackingNode.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/MatrixMath.h"
#include "eagleeye/common/EagleeyeTime.h"


namespace eagleeye{
SimRPNTrackingNode::SimRPNTrackingNode(){
    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
    // 设置输出端口(端口0)及携带数据类型(TargetT)
    this->setOutputPort(new ImageSignal<float>, OUTPUT_PORT_BOX);

    // 设置输入端口（拥有2个输入端口）
	this->setNumberOfInputSignals(2);

    EAGLEEYE_LOGD("initialize target stage model");
    this->m_model = new ModelRun("target_stage",
                                 "GPU",
                                 std::vector<std::string>{"input_node_t"},
                                 std::vector<std::vector<int64_t>>{std::vector<int64_t>{1,103,103,3}},
                                 std::vector<std::string>{"tck","trk"},
                                 std::vector<std::vector<int64_t>>{std::vector<int64_t>{1,128,1,1},
                                                                    std::vector<int64_t>{1,128,1,1}});

    this->m_model->initialize();

    EAGLEEYE_LOGD("initialize search stage model");
    this->m_tracking_model = new ModelRun("search_stage",
                                          "GPU",
                                          std::vector<std::string>{"input_node_d","input_node_tck","input_node_trk"},
                                          std::vector<std::vector<int64_t>>{std::vector<int64_t>{1,183,183,3},
                                                                            std::vector<int64_t>{1,128,1,1},
                                                                            std::vector<int64_t>{1,128,1,1}},
                                          std::vector<std::string>{"output_node_cls_nhwc","output_node_reg_nhwc"},
                                          std::vector<std::vector<int64_t>>{std::vector<int64_t>{1,11,11,10},
                                                                            std::vector<int64_t>{1,11,11,20}});
    this->m_tracking_model->initialize();
    m_last_box = Matrix<float>(1,6);
    m_last_center = Matrix<float>(1,2);
    m_init_box = Matrix<float>(1,4);

    this->m_instance_size = 183;
    this->m_exemplar_size = 103;
    this->m_anchor_stride = 8;
    this->m_track_base_size = 0;
    this->m_anchor_num = 5;
    this->m_adaptive_lr = 0.3;
    this->m_anchors = this->generateAnchor();
    this->TRACK_PENALTY_K = 0.16;
    this->TRACK_WINDOW_INFLUENCE = 0.40;

    Matrix<float> h = hanning(11);
    Matrix<float> w = outer(h,h);
    this->m_window = tile(w.flatten(), std::vector<int>{this->m_anchor_num});
    this->m_window = this->m_window.t();

    // set writable path
    EAGLEEYE_MONITOR_VAR(std::string, setWritablePath, getWritablePath, "writablepath","","");
}   

SimRPNTrackingNode::~SimRPNTrackingNode(){
    delete this->m_model;
    delete this->m_tracking_model;
} 

void SimRPNTrackingNode::executeNodeInfo(){
    //get input image signal
	ImageSignal<Array<unsigned char, 3>>* input_img_sig = dynamic_cast<ImageSignal<Array<unsigned char, 3>>*>(this->m_input_signals[0]);
    ImageSignal<float>* target_box_sig = dynamic_cast<ImageSignal<float>*>(this->m_input_signals[1]);
    ImageSignal<float>* tracking_box_sig = dynamic_cast<ImageSignal<float>*>(this->m_output_signals[0]);
	if (!input_img_sig || !target_box_sig){
		EAGLEEYE_LOGE("input image is not correct...\n");
		return;
	}

    Matrix<Array<unsigned char, 3>> image = input_img_sig->getData();
    Matrix<float> target_box = target_box_sig->getData();
    // extract target and resize to 103,103
    // extract target feature
    if(target_box.rows() > 0 && target_box.cols() > 0){
        int x = target_box.at(0,0);
        int y = target_box.at(0,1);
        int w = target_box.at(0,2);
        int h = target_box.at(0,3);

        int rows = image.rows();
        int cols = image.cols();
        int roi[4] = {x,y,w,h};
        Matrix<Array<unsigned char, 3>> template_image = this->getTemplateImage(image, roi);
        // cv::Mat cc(template_image.rows(), template_image.cols(), CV_8UC3, template_image.dataptr());
        // cv::imwrite("./test.png",cc);

        Matrix<Array<float,3>> template_image_f(template_image.rows(), template_image.cols());
        for(int i=0; i<template_image.rows(); ++i){
            unsigned char* template_image_ptr = (unsigned char*)template_image.row(i);
            float* template_image_f_ptr = (float*)template_image_f.row(i);
            for(int j=0; j<template_image.cols(); ++j){
                template_image_f_ptr[j*3] = template_image_ptr[j*3];                // r
                template_image_f_ptr[j*3+1] = template_image_ptr[j*3+1];            // g
                template_image_f_ptr[j*3+2] = template_image_ptr[j*3+2];            // b
            }
        }

        std::map<std::string, unsigned char*> inputs;
        inputs["input_node_t"] = (unsigned char*)template_image_f.dataptr();
        std::map<std::string, unsigned char*> outputs;
        EAGLEEYE_LOGD("running simrpn target network");
        long start_time = EagleeyeTime::getCurrentTime();
        bool status = this->m_model->run(inputs, outputs);
        long end_time = EagleeyeTime::getCurrentTime();
        EAGLEEYE_LOGD("finish simrpn target network by %d ms", int(end_time-start_time));

        // template convolution kernel
        this->m_target_tck = Tensor<float>(std::vector<int64_t>{1,128,1,1}, outputs["tck"], true);
        this->m_target_trk = Tensor<float>(std::vector<int64_t>{1,128,1,1}, outputs["trk"], true);

        // last box and center
        m_last_box.at(0,0) = x;
        m_last_box.at(0,1) = y;
        m_last_box.at(0,2) = w;
        m_last_box.at(0,3) = h;
        m_last_box.at(0,4) = 1.0f;
        m_last_box.at(0,5) = 1.0f;
        tracking_box_sig->setData(m_last_box);

        m_last_center.at(0,0) = (x + w / 2.0f);
        m_last_center.at(0,1) = (y + h / 2.0f);

        // template object box
        m_init_box.at(0, 0) = x;
        m_init_box.at(0, 1) = y;
        m_init_box.at(0, 2) = w;
        m_init_box.at(0, 3) = h;
        return;
    }

    int last_roi[4] = {int(m_last_box.at(0,0)),
                       int(m_last_box.at(0,1)),
                       int(m_last_box.at(0,2)),
                       int(m_last_box.at(0,3))};
    float scale = 1.0f;
    EAGLEEYE_TIME_START(getdetectimage);
    Matrix<Array<unsigned char, 3>> search_image = this->getDetectImage(image, last_roi, scale);
    EAGLEEYE_TIME_END(getdetectimage);

    Matrix<Array<float, 3>> search_image_f(search_image.rows(), search_image.cols());
    for(int i=0; i<search_image.rows(); ++i){
        unsigned char* search_image_ptr = (unsigned char*)search_image.row(i);
        float* search_image_f_ptr = (float*)search_image_f.row(i);
        for(int j=0; j<search_image_f.cols(); ++j){
            search_image_f_ptr[j*3] = search_image_ptr[j*3];                    // r
            search_image_f_ptr[j*3+1] = search_image_ptr[j*3+1];                // g
            search_image_f_ptr[j*3+2] = search_image_ptr[j*3+2];                // b
        }
    }

    std::map<std::string, unsigned char*> search_inputs;
    search_inputs["input_node_d"] = (unsigned char*)search_image_f.dataptr();
    search_inputs["input_node_tck"] = (unsigned char*)m_target_tck.dataptr();
    search_inputs["input_node_trk"] = (unsigned char*)m_target_trk.dataptr();
    std::map<std::string, unsigned char*> search_outputs;
    EAGLEEYE_LOGD("running simrpn search network");
    long start_time = EagleeyeTime::getCurrentTime();
    bool status = this->m_tracking_model->run(search_inputs, search_outputs);
    long end_time = EagleeyeTime::getCurrentTime();
    EAGLEEYE_LOGD("finish simrpn search network by %d ms", int(end_time-start_time));

    Tensor<float> cls_output = Tensor<float>(std::vector<int64_t>{1,11,11,10},search_outputs["output_node_cls_nhwc"],true);
    Tensor<float> reg_output = Tensor<float>(std::vector<int64_t>{1,11,11,20},search_outputs["output_node_reg_nhwc"],true);
    
    Matrix<float> _cls_output(121, 10, cls_output.dataptr(), false);
    Matrix<float> _reg_output(121, 20, reg_output.dataptr(), false);

    Matrix<float> cls_output_c(605,2);
    Matrix<float> reg_output_c(605,4);
    for (int i = 0; i < 121; i++)
    {
        for(int j=0; j<5; ++j){
            // cls
            cls_output_c.at(j*121+i,0) = _cls_output.at(i, 0*5+j);
            cls_output_c.at(j*121+i,1) = _cls_output.at(i, 1*5+j);

            // reg
            reg_output_c.at(j*121+i,0) = _reg_output.at(i, 0*5+j);
            reg_output_c.at(j*121+i,1) = _reg_output.at(i, 1*5+j);
            reg_output_c.at(j*121+i,2) = _reg_output.at(i, 2*5+j);
            reg_output_c.at(j*121+i,3) = _reg_output.at(i, 3*5+j);
        }
    }
    _cls_output = cls_output_c;
    _reg_output = reg_output_c;
    
    // decoder to coordinate
    EAGLEEYE_TIME_START(decoder);
    Matrix<float> predict_box = convertBbox(_reg_output, this->m_anchors);
    predict_box = predict_box / scale;
    _cls_output = msoftmax(_cls_output);
    Matrix<float> positive_cls_score = _cls_output(Range(0, 605), Range(1,2));
    Matrix<float> penalty_score = smooth(predict_box, positive_cls_score);
    Matrix<int> best_idx = argmax(penalty_score, 0);
    int best_anchor_idx = best_idx.at(0,0);
    EAGLEEYE_TIME_END(decoder);

    // 上一帧中目标中心位置
    int last_cx = last_roi[0] + last_roi[2]/2.0f;
    int last_cy = last_roi[1] + last_roi[3]/2.0f;

    float cx = predict_box.at(best_anchor_idx, 0);
    float cy = predict_box.at(best_anchor_idx, 1);
    cx = cx + last_cx;
    cy = cy + last_cy;

    // // TRACK_LR = 0.30
    float TRACK_LR = 0.30f;
    // float lr = penalty_score.at(best_anchor_idx,0)*TRACK_LR;
    // float smooth_width = m_last_box.at(0,2) * (1 - lr) + predict_box.at(best_anchor_idx, 2) * lr;
    // float smooth_height = m_last_box.at(0,3) * (1 - lr) + predict_box.at(best_anchor_idx, 3) * lr;
    float smooth_width = m_last_box.at(0,2);
    float smooth_height = m_last_box.at(0,3);
    // float smooth_width = w;
    // float smooth_height = h;

    cx = EAGLEEYE_MIN(cx, image.cols());
    cx = EAGLEEYE_MAX(0, cx);

    cy = EAGLEEYE_MIN(cy, image.rows());
    cy = EAGLEEYE_MAX(0, cy);

    smooth_width = EAGLEEYE_MIN(smooth_width, image.cols());
    smooth_width = EAGLEEYE_MAX(10, smooth_width);

    smooth_height = EAGLEEYE_MIN(smooth_height, image.rows());
    smooth_height = EAGLEEYE_MAX(10, smooth_height);

    // output tracking bbox
    m_last_box.at(0,0) = cx - smooth_width / 2.0f;
    m_last_box.at(0,1) = cy - smooth_height / 2.0f;
    m_last_box.at(0,2) = smooth_width;
    m_last_box.at(0,3) = smooth_height;
    m_last_box.at(0,4) = positive_cls_score.at(best_anchor_idx,0);
    m_last_box.at(0,5) = penalty_score.at(best_anchor_idx,0);
    tracking_box_sig->setData(m_last_box);
}

bool SimRPNTrackingNode::selfcheck(){
    return true;
}

void SimRPNTrackingNode::setWritablePath(std::string path){
    this->m_model->setWritablePath(path);
    this->m_tracking_model->setWritablePath(path);
}

void SimRPNTrackingNode::getWritablePath(std::string& path){
    path = this->m_model->getWritablePath();
}

Matrix<Array<unsigned char, 3>> SimRPNTrackingNode::getTemplateImage(Matrix<Array<unsigned char, 3>> frame, int* roi){
    // roi: x,y,w,h
    int c_x = roi[0] + roi[2]/2;
    int c_y = roi[1] + roi[3]/2;

    int w_z = roi[2] + (roi[2] + roi[3])/2.0f;
    int h_z = roi[3] + (roi[2] + roi[3])/2.0f;
    int s_z = round(sqrt(w_z*h_z));

    int rows = frame.rows();
    int cols = frame.cols();
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    int count = 0;
    for(int i=0; i<rows; ++i){
        unsigned char* ptr = (unsigned char*)frame.row(i);
        for(int j=0; j<cols; ++j){
            r = (r * count + ptr[j*3]) / float(count + 1);
            g = (g * count + ptr[j*3+1]) / float(count + 1);
            b = (b * count + ptr[j*3+2]) / float(count + 1);

            count += 1;
        }
    }

    return this->getSubWindow(frame, roi, c_x, c_y, s_z,true);
}

Matrix<Array<unsigned char, 3>> SimRPNTrackingNode::getDetectImage(Matrix<Array<unsigned char, 3>> frame, 
                                                                   int* roi, 
                                                                   float& scale){
    // roi: x,y,w,h
    int c_x = roi[0] + roi[2]/2;
    int c_y = roi[1] + roi[3]/2;

    int w_z = roi[2] + (roi[2] + roi[3])/2;
    int h_z = roi[3] + (roi[2] + roi[3])/2;
    int s_z = round(sqrt(w_z*h_z));

    int s_x = round(s_z * ((float)this->m_instance_size / (float)this->m_exemplar_size));
    scale = (float)this->m_exemplar_size / s_z;
    return this->getSubWindow(frame,roi,c_x,c_y,s_x, false);
}

Matrix<Array<unsigned char, 3>> SimRPNTrackingNode::getSubWindow(Matrix<Array<unsigned char, 3>> frame, 
                                int* roi,
                                int c_x,
                                int c_y,
                                int s_z,
                                bool is_template){
    EAGLEEYE_TIME_START(presubwindow);
    int rows = frame.rows();
    int cols = frame.cols();
    // padding
    int c = (s_z + 1) / 2;
    int context_xmin = floor(c_x - c + 0.5);
    int context_xmax = context_xmin + s_z - 1; 
    int context_ymin = floor(c_y - c + 0.5);
    int context_ymax = context_ymin + s_z - 1;

    int left_pad = int(EAGLEEYE_MAX(0.0f, -context_xmin));
    int top_pad = int(EAGLEEYE_MAX(0.0f, -context_ymin));

    int right_pad = int(EAGLEEYE_MAX(0.0f, context_xmax - cols + 1));
    int bottom_pad = int(EAGLEEYE_MAX(0.0f, context_ymax - rows + 1));

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

    // resize to
    if(is_template){
        Matrix<Array<unsigned char,3>> template_img = 
                    resize(local_window, 
                            this->m_exemplar_size,
                            this->m_exemplar_size, 
                            BILINEAR_INTERPOLATION);
        return template_img;
    }
    else{
        EAGLEEYE_TIME_START(resize);
        Matrix<Array<unsigned char,3>> template_img = 
                    resize(local_window, 
                            this->m_instance_size,
                            this->m_instance_size,
                            BILINEAR_INTERPOLATION);
        EAGLEEYE_TIME_END(resize);
        return template_img;
    }
}

Matrix<float> SimRPNTrackingNode::generateAnchor(){
    int score_size = (this->m_instance_size - this->m_exemplar_size)/this->m_anchor_stride + 1 + this->m_track_base_size;
    int stride = 8;
    std::vector<float> ratios{0.33,0.5,1,2,3};
    std::vector<float> scales{8};

    int anchor_num = ratios.size() * scales.size();
    Matrix<float> anchors(anchor_num, 4);

    int size = stride * stride;
    int count = 0;
    for(int r_index=0; r_index<ratios.size(); ++r_index){
        int ws = int(sqrt(size * 1.0f / ratios[r_index]));
        int hs = ws * ratios[r_index];

        for(int s_index=0; s_index<scales.size(); ++s_index){
            float w = ws * scales[s_index];
            float h = hs * scales[s_index];
            anchors.at(count, 0) = -w*0.5f;
            anchors.at(count, 1) = -h*0.5f;
            anchors.at(count, 2) = w*0.5f;
            anchors.at(count, 3) = h*0.5f;

            count += 1;
        }
    }   
    
    Matrix<float> x1 = anchors(Range(0, anchors.rows()), Range(0,1));
    Matrix<float> y1 = anchors(Range(0, anchors.rows()), Range(1,2));
    Matrix<float> x2 = anchors(Range(0, anchors.rows()), Range(2,3));
    Matrix<float> y2 = anchors(Range(0, anchors.rows()), Range(3,4));

    Matrix<float> cx = (x1+x2)/2.0f;
    Matrix<float> cy = (y1+y2)/2.0f;
    Matrix<float> w = x2-x1;
    Matrix<float> h = y2-y1;

    std::vector<Matrix<float>> mm{cx,cy,w,h};
    anchors = concat<float>(mm,1);
    anchors = tile<float>(anchors, std::vector<int>{score_size*score_size});
    anchors = anchors.reshape(anchors.rows()*anchors.cols()/4,4);

    int roi = -(score_size/2) * stride;
    Matrix<float> xx(1,score_size);
    Matrix<float> yy(1,score_size);
    for(int i=0; i<score_size; ++i){
        xx.at(i) = roi + stride * i;
        yy.at(i) = roi + stride * i;
    }

    Matrix<float> xg;
    Matrix<float> yg;
    meshgrid(xx,yy,xg,yg);

    xg = tile(xg.flatten(), std::vector<int>{anchor_num, 1});
    xg = xg.flatten();

    yg = tile(yg.flatten(), std::vector<int>{anchor_num, 1});
    yg = yg.flatten();

    anchors(Range(0, anchors.rows()),Range(0,1)).copy(xg.t());
    anchors(Range(0, anchors.rows()),Range(1,2)).copy(yg.t());
    return anchors;
}

Matrix<float> SimRPNTrackingNode::convertBbox(Matrix<float> delta, 
                                              Matrix<float> anchor){
    int rows = delta.rows();
    int cols = delta.cols();
    Matrix<float> converted_bbox(rows, cols);
    Matrix<float> a = delta(Range(0,rows),Range(0,1)).mul(anchor(Range(0,rows), Range(2,3))) + anchor(Range(0,rows),Range(0,1));
    converted_bbox(Range(0,rows),Range(0,1)).copy(a);

    Matrix<float> b = delta(Range(0,rows),Range(1,2)).mul(anchor(Range(0,rows), Range(3,4))) + anchor(Range(0,rows),Range(1,2));
    converted_bbox(Range(0,rows),Range(1,2)).copy(b);

    Matrix<float> c = mexp(delta(Range(0,rows),Range(2,3))).mul(anchor(Range(0,rows), Range(2,3)));
    converted_bbox(Range(0,rows),Range(2,3)).copy(c);

    Matrix<float> d = mexp(delta(Range(0,rows),Range(3,4))).mul(anchor(Range(0,rows), Range(3,4)));
    converted_bbox(Range(0,rows),Range(3,4)).copy(d);
    return converted_bbox;
}

Matrix<float> SimRPNTrackingNode::smooth(Matrix<float> predict_box, Matrix<float> predict_cls){
    // smooth
    // scale penalty
    int rows = predict_box.rows();
    int cols = predict_box.cols();
    Matrix<float> predict_pad = (predict_box(Range(0, rows),Range(2,3)) + predict_box(Range(0,rows), Range(3,4))) * 0.5f;
    Matrix<float> predict_sz = msqrt((predict_box(Range(0, rows),Range(2,3)) + predict_pad).mul(predict_box(Range(0,rows), Range(3,4)) + predict_pad));

    float last_pad = (this->m_last_box.at(0,2) + this->m_last_box.at(0,3))*0.5f;
    float last_sz = sqrt((this->m_last_box.at(0,2)+last_pad)*(this->m_last_box.at(0,3)+last_pad));

    Matrix<float> scale_penalty(rows, 1);
    for(int i=0; i<rows; ++i){
        float s = predict_sz.at(i,0) / last_sz;
        scale_penalty.at(i,0) = EAGLEEYE_MAX(s, 1.0f/s);
    }

    //  # aspect ratio penalty
    //  r_c = change((float(size[0]) / float(size[1])) /
    //               (pred_bbox[2, :] / pred_bbox[3, :]))

    Matrix<float> aspect_penalty(rows, 1);
    Matrix<float> predict_r = predict_box(Range(0, rows),Range(2,3)).div(predict_box(Range(0, rows), Range(3,4)));
    float last_r = this->m_last_box.at(0,2)/this->m_last_box.at(0,3);
    for(int i=0; i<rows; ++i){
        float r = last_r / predict_r.at(i,0);
        aspect_penalty.at(i,0) = EAGLEEYE_MAX(r, 1.0f/r);
    }

    //  penalty = np.exp(-(r_c * s_c - 1) * TRACK_PENALTY_K)
    Matrix<float> penalty = mexp((aspect_penalty.mul(scale_penalty) - 1.0f)*(-TRACK_PENALTY_K));
    Matrix<float> pscore = penalty.mul(predict_cls);

    //  # window penalty
    //  pscore = pscore * (1 - TRACK_WINDOW_INFLUENCE) + \
    //           window * TRACK_WINDOW_INFLUENCE

    pscore = pscore * (1.0f-TRACK_WINDOW_INFLUENCE) + this->m_window * TRACK_WINDOW_INFLUENCE;
    return pscore;
}

}