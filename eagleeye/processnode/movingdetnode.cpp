#include "eagleeye/processnode/movingdetnode.h"
#include "eagleeye/framework/pipeline/AnyMonitor.h"
#include "eagleeye/common/EagleeyeTime.h"
#include <cmath>
#include <random>
#include <cstdint>
#include "eagleeye/algorithm/llsp.h"
#include "eagleeye/basic/MetaOperation.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/algorithm/filter.h"
#include "eagleeye/common/EagleeyeIO.h"

namespace eagleeye{
MovingDetNode::MovingDetNode(){
    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
    // 设置输出端口(端口0)及携带数据类型(TargetT)
    this->setOutputPort(new ImageSignal<float>, OUTPUT_PORT_BOX);

    // 设置输入端口
    // port 0: frame
    // port 1: optical flow
	this->setNumberOfInputSignals(2);

    std::cout<<"init moving detnode"<<std::endl;

    this->m_p = 0.6;
    this->m_alpha = 3.23;
    this->m_beta = 2.3;
    this->m_smooth_diff_sig = 20.55;
    this->m_smooth_spatial_sig = 6.06;
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
    // OpenCLEnv::setWritablePath("/sdcard/");
#endif
    m_h_estimator = new RANSAC<HomographyModel>();
    m_h_estimator->initialize(1.0f, 50);
    EAGLEEYE_MONITOR_VAR(float,setP,getP,"P","0.1","1.0");
    EAGLEEYE_MONITOR_VAR(float, setAlpha,getAlpha,"alpha", "0.5", "20.0");
    EAGLEEYE_MONITOR_VAR(float, setBeta,getBeta,"beta", "0.5", "10.0");
    EAGLEEYE_MONITOR_VAR(float, setSmoothDiff,getSmoothDiff,"SD","3", "30");
    EAGLEEYE_MONITOR_VAR(float, setSmoothSpatial, getSmoothSpatial, "SP", "3", "20");
}   

MovingDetNode::~MovingDetNode(){
    delete m_h_estimator;
}

void MovingDetNode::executeNodeInfo(){
    InputSigType* frame_sig = this->getInputImageSignal(0);
    ImageSignal<Array<float,2>>* optical_flow_sig = (ImageSignal<Array<float,2>>*)this->m_input_signals[1];
    Matrix<Array<unsigned char,3>> frame = frame_sig->getData();
    Matrix<Array<float,2>> optical_flow = optical_flow_sig->getData();
    Matrix<unsigned char> gray = frame.transform(RGB2GRAY<unsigned char>());
    int frame_rows = frame.rows();
    int frame_cols = frame.cols();

    OutputSigType* score_sig = this->getOutputImageSignal(0);
    if(m_pre_frame.isempty()){
        this->m_pre_frame = gray;
        score_sig->setData(Matrix<float>());
        return;
    }

    // 2.step compute background model
    // finding homography transfer matrix
    // H x pre_points = cur_points
    EAGLEEYE_TIME_START(prefindHomography);
    std::vector<std::shared_ptr<AbstractParameter>> CandPoints;
    int point_num = this->m_pre_frame.rows() * this->m_pre_frame.cols();
    for(int i=0; i<this->m_pre_frame.rows(); i+=3){
        Array<float,2>* opt_flow_ptr = optical_flow.row(i);
        for(int j=0; j<this->m_pre_frame.cols(); j+=3){
        	std::shared_ptr<AbstractParameter> pt = std::make_shared<HomographyParam>(j,i,1,j+opt_flow_ptr[j][0],i+opt_flow_ptr[j][1],1);
            CandPoints.push_back(pt);
        }
    }
    EAGLEEYE_TIME_END(prefindHomography);

    EAGLEEYE_TIME_START(findHomography);
    m_h_estimator->estimate(CandPoints);
    
    float* H_ptr = m_h_estimator->getBestModel().dataptr();
    Matrix<float> H(3,3,H_ptr);
    Matrix<float> translation, scale;
    float rotation;
    HomographyModel::decompose(H, translation, scale, rotation);

    float translation_x = translation.at(0,0); float translation_y = translation.at(1,0);
    EAGLEEYE_LOGD("translation delta x %f delta y %f", translation_x, translation_y);
    EAGLEEYE_LOGD("sacle x %f y %f", scale.at(0,0), scale.at(1,0));

    EAGLEEYE_TIME_END(findHomography);
    // background
    Matrix<float> pre_points_mat(3, this->m_pre_frame.rows()*this->m_pre_frame.cols());
    for(int i=0; i<this->m_pre_frame.rows(); ++i){
        for(int j=0; j<this->m_pre_frame.cols(); ++j){
            pre_points_mat.at(0,i*this->m_pre_frame.cols()+j) = j;
            pre_points_mat.at(1,i*this->m_pre_frame.cols()+j) = i;
            pre_points_mat.at(2,i*this->m_pre_frame.cols()+j) = 1;
        }
    }

    Matrix<float> approx_flow =  H*pre_points_mat;
    for(int i=0; i<this->m_pre_frame.rows()*this->m_pre_frame.cols(); ++i){
        approx_flow.at(0,i) /= approx_flow.at(2,i);
        approx_flow.at(1,i) /= approx_flow.at(2,i);
        approx_flow.at(2,i) /= approx_flow.at(2,i);
    }
    approx_flow = approx_flow - pre_points_mat;

    // 3.step extract foreground object
    float thres = this->m_alpha + this->m_beta * sqrt(pow(translation_x,2)+pow(translation_y,2));
    float mean_diff = 0.0f;
    float max_diff = 0.0f;
    Matrix<float> moving_diff(frame_rows, frame_cols);
    Matrix<float> moving_heatmap(frame_rows, frame_cols);
    for(int i=0; i<frame_rows; ++i){
        Array<float,2>* opt_flow_ptr = optical_flow.row(i);
        for(int j=0; j<frame_cols; ++j){
            float x_diff = approx_flow.at(0, i*frame_cols+j) - opt_flow_ptr[j][0];
            float y_diff = approx_flow.at(1, i*frame_cols+j) - opt_flow_ptr[j][1];
            float d = sqrt(pow(x_diff,2)+pow(y_diff,2));
            moving_diff.at(i,j) = d;
            mean_diff += d;
            max_diff = eagleeye_max(max_diff, d);
        }
    }
    mean_diff /= (frame_rows*frame_cols);
    
    // filter moving diff
    Filter filter;
    filter.setSigmaI(this->m_smooth_diff_sig);
    filter.setSigmaS(this->m_smooth_spatial_sig);
    Matrix<float> smoothing_moving_diff;
    Matrix<float> content_reference = gray.transform<float>();
    filter.crossFastBF(moving_diff, content_reference, smoothing_moving_diff);
    moving_diff = smoothing_moving_diff;

    for(int i=0; i<frame_rows; ++i){
        float* moving_diff_ptr = moving_diff.row(i);
        Array<float,2>* opt_flow_ptr = optical_flow.row(i);
        for(int j=0; j<frame_cols; ++j){
            float flow_x = opt_flow_ptr[j][0];
            float flow_y = opt_flow_ptr[j][1];
            float flow_magitude = sqrt(flow_x*flow_x+flow_y*flow_y);
            if(moving_diff_ptr[j] > (max_diff-mean_diff)*this->m_p+mean_diff && flow_magitude > thres){
                moving_heatmap.at(i,j) = moving_diff_ptr[j];
            }
        }
    }
    
    // Matrix<float> norm_moving_heatmap = moving_heatmap.transform(NormalizeMINMAX<float, float>(0.0f,1.0f));
    score_sig->setData(moving_heatmap);

    // update pre_frame
    m_pre_frame = gray;
}

void MovingDetNode::setAlpha(float a1){
    this->m_alpha = a1;
    modified();
}
void MovingDetNode::getAlpha(float& a1){
    a1 = this->m_alpha;
}

void MovingDetNode::setBeta(float a2){
    this->m_beta = a2;
    modified();
}
void MovingDetNode::getBeta(float& a2){
    a2 = this->m_beta;
}

void MovingDetNode::setP(float p){
    this->m_p = p;
    modified();
}
void MovingDetNode::getP(float& p){
    p = this->m_p;
}

void MovingDetNode::setSmoothDiff(float diff_sig){
    this->m_smooth_diff_sig = diff_sig;
    modified();
    EAGLEEYE_LOGD("set smooth diff %f", diff_sig);
}
void MovingDetNode::getSmoothDiff(float& diff_sig){
    diff_sig = this->m_smooth_diff_sig;
}

void MovingDetNode::setSmoothSpatial(float spatial_sig){
    this->m_smooth_spatial_sig = spatial_sig;
    modified();
    EAGLEEYE_LOGD("set smooth spatial %f", spatial_sig);
}
void MovingDetNode::getSmoothSpatial(float& spatial_sig){
    spatial_sig = this->m_smooth_spatial_sig;
}
}