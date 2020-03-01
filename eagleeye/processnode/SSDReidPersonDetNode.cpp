#include "eagleeye/processnode/SSDReidPersonDetNode.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/basic/MatrixMath.h"
#include "eagleeye/common/EagleeyeTime.h"
#include "eagleeye/algorithm/nms.h"
#include "eagleeye/tensorop/imageop.h"
#include "eagleeye/tensorop/tensorop.h"

namespace eagleeye{
SSDReIDPersonDetNode::SSDReIDPersonDetNode(bool enable_reid):
    m_enable_reid(enable_reid){
    this->m_det = new ModelRun("person_det_dsp", 
                                "DSP",
                                std::vector<std::string>(),
                                std::vector<std::vector<int64_t>>(),
                                std::vector<std::string>(),
                                std::vector<std::vector<int64_t>>(),
                                -1,
                                0,
                                "/data/local/tmp/GGT");
    // 设置输出端口（拥有2个输出端口）
    this->setNumberOfOutputSignals(2);
    // 设置输出端口(端口0)及携带数据类型(TargetT)
    this->setOutputPort(new ImageSignal<float>, OUTPUT_PORT_BOX);
    this->setOutputPort(new ImageSignal<float>, OUTPUT_PORT_REIDFEATURE);

    // 设置输入端口（拥有1个输入端口）
    // port 0: frame
	this->setNumberOfInputSignals(1);

    // 设置监控变量
    EAGLEEYE_MONITOR_VAR(std::string, setModelPath, getModelPath,"modelpath", "", "");
    EAGLEEYE_MONITOR_VAR(float, setNSMScoreThres, getNSMScoreThres, "nms_score_thres","0","1");
    EAGLEEYE_MONITOR_VAR(float, setIOUScoreThres, getIOUScoreThres, "nms_iou_thres","0","1");
    
    // stage 1: object detection 
    this->m_det->setInputNames(std::vector<std::string>{"input"});
    this->m_det->setInputShapes(std::vector<std::vector<int64_t>>{std::vector<int64_t>{1, 300, 300, 3}});
    this->m_h = 300;
    this->m_w = 300;
    this->m_max_num = 10;
    this->m_class_num = 4;
    this->m_nms_iou_thres = 0.3;
    this->m_nms_conf_thres = 0.3;

    this->m_output_names =  
                    std::vector<std::string>{"location_pred_0",
                                             "location_pred_1",
                                             "location_pred_2",
                                             "location_pred_3",
                                             "location_pred_4",
                                             "cls_pred_0",
                                             "cls_pred_1",
                                             "cls_pred_2",
                                             "cls_pred_3",
                                             "cls_pred_4",
                                             "feature_layer_0",
                                             "feature_layer_1",
                                             "feature_layer_2",
                                             "feature_layer_3",
                                             "feature_layer_4"};
    this->m_det->setOutputNames(this->m_output_names);

    this->m_output_shapes.push_back(std::vector<int64_t>{1, 38, 38, 4*4});
    this->m_output_shapes.push_back(std::vector<int64_t>{1, 19, 19, 6*4});
    this->m_output_shapes.push_back(std::vector<int64_t>{1, 10, 10, 6*4});
    this->m_output_shapes.push_back(std::vector<int64_t>{1,  5,  5, 6*4});
    this->m_output_shapes.push_back(std::vector<int64_t>{1,  3,  3, 6*4});

    this->m_output_shapes.push_back(std::vector<int64_t>{1, 38, 38, 4*this->m_class_num});
    this->m_output_shapes.push_back(std::vector<int64_t>{1, 19, 19, 6*this->m_class_num});
    this->m_output_shapes.push_back(std::vector<int64_t>{1, 10, 10, 6*this->m_class_num});
    this->m_output_shapes.push_back(std::vector<int64_t>{1,  5,  5, 6*this->m_class_num});
    this->m_output_shapes.push_back(std::vector<int64_t>{1,  3,  3, 6*this->m_class_num});
    
    this->m_output_shapes.push_back(std::vector<int64_t>{1, 38, 38, 256});
    this->m_output_shapes.push_back(std::vector<int64_t>{1, 19, 19, 256});
    this->m_output_shapes.push_back(std::vector<int64_t>{1, 10, 10, 256});
    this->m_output_shapes.push_back(std::vector<int64_t>{1,  5,  5, 256});
    this->m_output_shapes.push_back(std::vector<int64_t>{1,  3,  3, 256});                          
    this->m_det->setOutputShapes(this->m_output_shapes);

    // make compatibility with SNPE engine
    std::map<std::string, std::string> output_name_map;
    output_name_map["location_pred_0"] = "ssd300/multibox_head/rpn_regression_0/Conv2D";
    output_name_map["location_pred_1"] = "ssd300/multibox_head/rpn_regression_1/Conv2D";
    output_name_map["location_pred_2"] = "ssd300/multibox_head/rpn_regression_2/Conv2D";
    output_name_map["location_pred_3"] = "ssd300/multibox_head/rpn_regression_3/Conv2D";
    output_name_map["location_pred_4"] = "ssd300/multibox_head/rpn_regression_4/Conv2D";

    output_name_map["cls_pred_0"] = "ssd300/multibox_head/rpn_classification_0/Conv2D";
    output_name_map["cls_pred_1"] = "ssd300/multibox_head/rpn_classification_1/Conv2D";
    output_name_map["cls_pred_2"] = "ssd300/multibox_head/rpn_classification_2/Conv2D";
    output_name_map["cls_pred_3"] = "ssd300/multibox_head/rpn_classification_3/Conv2D";
    output_name_map["cls_pred_4"] = "ssd300/multibox_head/rpn_classification_4/Conv2D";

    output_name_map["feature_layer_0"] = "ssd300/reid_change_channel/Conv/Relu";
    output_name_map["feature_layer_1"] = "ssd300/Conv_9/Relu";
    output_name_map["feature_layer_2"] = "ssd300/Conv_8/Relu";
    output_name_map["feature_layer_3"] = "ssd300/Conv_7/Relu";
    output_name_map["feature_layer_4"] = "ssd300/Conv_6/Relu";
    this->m_det->setOutputNameMap(output_name_map);

    std::map<std::string, std::string> output_name_map2;
    output_name_map2["location_pred_0"] = "ssd300/multibox_head/rpn_regression_0/BiasAdd";
    output_name_map2["location_pred_1"] = "ssd300/multibox_head/rpn_regression_1/BiasAdd";
    output_name_map2["location_pred_2"] = "ssd300/multibox_head/rpn_regression_2/BiasAdd";
    output_name_map2["location_pred_3"] = "ssd300/multibox_head/rpn_regression_3/BiasAdd";
    output_name_map2["location_pred_4"] = "ssd300/multibox_head/rpn_regression_4/BiasAdd";

    output_name_map2["cls_pred_0"] = "ssd300/multibox_head/rpn_classification_0/BiasAdd";
    output_name_map2["cls_pred_1"] = "ssd300/multibox_head/rpn_classification_1/BiasAdd";
    output_name_map2["cls_pred_2"] = "ssd300/multibox_head/rpn_classification_2/BiasAdd";
    output_name_map2["cls_pred_3"] = "ssd300/multibox_head/rpn_classification_3/BiasAdd";
    output_name_map2["cls_pred_4"] = "ssd300/multibox_head/rpn_classification_4/BiasAdd";

    output_name_map2["feature_layer_0"] = "ssd300/reid_change_channel/Conv/Relu";
    output_name_map2["feature_layer_1"] = "ssd300/Conv_9/Relu";
    output_name_map2["feature_layer_2"] = "ssd300/Conv_8/Relu";
    output_name_map2["feature_layer_3"] = "ssd300/Conv_7/Relu";
    output_name_map2["feature_layer_4"] = "ssd300/Conv_6/Relu";
    this->m_det->setOutputNameMap2(output_name_map2);

    // get all fixed anchors
    this->m_anchors = getAllAnchors();
    // input model data
    m_model_image_f = Matrix<Array<float,3>>(this->m_h, this->m_w);

    if(this->m_enable_reid){
        // reid model
        this->m_reid = new ModelRun("person_reid_gpu", 
                                                "GPU",
                                                std::vector<std::string>{"input_reid"},
                                                std::vector<std::vector<int64_t>>{{-1, 7, 7, 256}},
                                                std::vector<std::string>{"person_feature"},
                                                std::vector<std::vector<int64_t>>{{-1, 1, 1, 256}},
                                                -1,
                                                0,
                                                "/data/local/tmp/GGT");
        // make compatibility with SNPE engine
        std::map<std::string, std::string> reid_output_name_map;
        reid_output_name_map["person_feature"] = "post_forward/ReidHead/Conv_1/Conv2D";
        this->m_reid->setOutputNameMap(reid_output_name_map);

        std::map<std::string, std::string> reid_output_name_map2;
        reid_output_name_map2["person_feature"] = "post_forward/ReidHead/Conv_1/BiasAdd";
        this->m_reid->setOutputNameMap2(reid_output_name_map2);
    }
}   

SSDReIDPersonDetNode::~SSDReIDPersonDetNode(){
    delete m_det;
    if(this->m_enable_reid){
        delete m_reid;
    }
}

void SSDReIDPersonDetNode::setModelPath(std::string path){
    EAGLEEYE_LOGD("initialize det model path %s", path.c_str());
    this->m_det->setWritablePath(path);
    this->m_det->initialize();

    if(this->m_enable_reid){
        EAGLEEYE_LOGD("initialize reid model");
        this->m_reid->setWritablePath(path);
        this->m_reid->initialize();
    }
}
void SSDReIDPersonDetNode::getModelPath(std::string& path){
    path = this->m_det->getWritablePath();
}

void SSDReIDPersonDetNode::setNSMScoreThres(float score){
    this->m_nms_conf_thres = score;
    this->modified();
}
void SSDReIDPersonDetNode::getNSMScoreThres(float& score){
    score = this->m_nms_conf_thres;
}

void SSDReIDPersonDetNode::setIOUScoreThres(float iou){
    this->m_nms_iou_thres = iou;
    this->modified();
}
void SSDReIDPersonDetNode::getIOUScoreThres(float& iou){
    iou = this->m_nms_iou_thres;
}

void SSDReIDPersonDetNode::executeNodeInfo(){
    // 0.step input/output signal
    // get input signal
	ImageSignal<Array<unsigned char, 3>>* frame_sig = (ImageSignal<Array<unsigned char, 3>>*)(this->m_input_signals[INPUT_PORT_FRAME]);
    Matrix<Array<unsigned char, 3>> frame = frame_sig->getData();

    int o_h = frame.rows();
    int o_w = frame.cols();

    // get output signal
    ImageSignal<float>* output_box_sig = (ImageSignal<float>*)(this->m_output_signals[OUTPUT_PORT_BOX]);
    ImageSignal<float>* output_reidfeature_sig = (ImageSignal<float>*)(this->m_output_signals[OUTPUT_PORT_REIDFEATURE]);

    // preprocess
    EAGLEEYE_TIME_START(preprocess);
    EAGLEEYE_TIME_START(preprocess_resize);
    Matrix<Array<unsigned char,3>> model_image = resize(frame, this->m_h, this->m_w, BILINEAR_INTERPOLATION);
    EAGLEEYE_TIME_END(preprocess_resize);
    unsigned char* model_image_ptr = (unsigned char*)model_image.dataptr();
    float* model_image_f_ptr = (float*)m_model_image_f.dataptr();

	// preprocess frame
    int model_h = this->m_h;
    int model_w = this->m_w;
#pragma omp parallel for    
    for(int i=0; i<model_h; ++i){
        for(int j=0; j<model_w; ++j){
            model_image_f_ptr[(i*model_w+j)*3] = (model_image_ptr[(i*model_w+j)*3] - 123.68) / 255.0;
            model_image_f_ptr[(i*model_w+j)*3+1] = (model_image_ptr[(i*model_w+j)*3+1] - 116.78) / 255.0;
            model_image_f_ptr[(i*model_w+j)*3+2] = (model_image_ptr[(i*model_w+j)*3+2] - 103.94) / 255.0;
        }
    }
    EAGLEEYE_TIME_END(preprocess);

    // stage 1: running object detection
    std::map<std::string, unsigned char*> inputs;
    inputs["input"] = (unsigned char*)model_image_f_ptr;
    std::map<std::string, unsigned char*> outputs;
    EAGLEEYE_TIME_START(objectdetrun);
    bool status = this->m_det->run(inputs, outputs);
    EAGLEEYE_TIME_END(objectdetrun);

    if(!status){
        EAGLEEYE_LOGE("fail to run SSDReidPersonDetNode model");
        return;
    }

    EAGLEEYE_TIME_START(decoderdet);
    std::vector<Matrix<float>> pl{Matrix<float>(38*38*4, 4,(void*)outputs[this->m_det->getOutputNames()[0]]),
                                  Matrix<float>(19*19*6, 4,(void*)outputs[this->m_det->getOutputNames()[1]]),
                                  Matrix<float>(10*10*6, 4,(void*)outputs[this->m_det->getOutputNames()[2]]),
                                  Matrix<float>( 5* 5*6, 4,(void*)outputs[this->m_det->getOutputNames()[3]]),
                                  Matrix<float>( 3* 3*6, 4,(void*)outputs[this->m_det->getOutputNames()[4]])};               
    Matrix<float> prediction_loc = concat(pl, 0);

    std::vector<Matrix<float>> pc{Matrix<float>(38*38*4, this->m_class_num,(void*)outputs[this->m_det->getOutputNames()[5]]),
                                  Matrix<float>(19*19*6, this->m_class_num,(void*)outputs[this->m_det->getOutputNames()[6]]),
                                  Matrix<float>(10*10*6, this->m_class_num,(void*)outputs[this->m_det->getOutputNames()[7]]),
                                  Matrix<float>( 5* 5*6, this->m_class_num,(void*)outputs[this->m_det->getOutputNames()[8]]),
                                  Matrix<float>( 3* 3*6, this->m_class_num,(void*)outputs[this->m_det->getOutputNames()[9]])};
    Matrix<float> prediction_cls = concat(pc, 0);

    int total_num = prediction_cls.rows();
    Matrix<float> object_cls = prediction_cls(Range(0, total_num), Range(0,this->m_class_num));
    EAGLEEYE_TIME_START(det_softmax);
    Matrix<float> object_cls_softmax = msoftmax(object_cls);
    EAGLEEYE_TIME_END(det_softmax);

    EAGLEEYE_TIME_START(nms_outer);
    std::vector<Matrix<float>> classes_selected_bboxes;
    for(int c_i = 1; c_i < 2; ++c_i){
        EAGLEEYE_TIME_START(DP);
        Matrix<float> obj_boxes = this->decodePosition(prediction_loc, 
                                                        object_cls_softmax(Range(0, total_num), Range(c_i,c_i+1)), 
                                                        o_h, 
                                                        o_w);
        EAGLEEYE_TIME_END(DP);                                                        
        
        int current_num = obj_boxes.rows();
        Matrix<float> candidates(current_num, 6);
        if(current_num > 0){
            candidates(Range(0, current_num), Range(0,5)).copy(obj_boxes);
            candidates(Range(0, current_num), Range(5,6)) = c_i;
            EAGLEEYE_TIME_START(nms);
            std::vector<unsigned int> keep = nms(candidates, this->m_nms_iou_thres, this->m_nms_conf_thres);
            EAGLEEYE_TIME_END(nms);
            Matrix<float> selected_bboxes = candidates.select(keep);
            classes_selected_bboxes.push_back(selected_bboxes);
        }
    }
    EAGLEEYE_TIME_END(nms_outer);

    Matrix<float> classes_bboxes = classes_selected_bboxes[0];
    int keeped_num = classes_bboxes.rows();
    Matrix<float> keeped_boxes = Matrix<float>(keeped_num, 6);
    for(int b_i=0; b_i<keeped_num; ++b_i){
        keeped_boxes.at(b_i, 0) = classes_bboxes.at(b_i,1);
        keeped_boxes.at(b_i, 1) = classes_bboxes.at(b_i,0);
        keeped_boxes.at(b_i, 2) = classes_bboxes.at(b_i,3) - classes_bboxes.at(b_i,1);
        keeped_boxes.at(b_i, 3) = classes_bboxes.at(b_i,2) - classes_bboxes.at(b_i,0);
        keeped_boxes.at(b_i, 4) = classes_bboxes.at(b_i,4);
        keeped_boxes.at(b_i, 5) = 1;
    }
    EAGLEEYE_TIME_END(decoderdet);
    // output preson bbox
    output_box_sig->setData(keeped_boxes);
    if(!this->m_enable_reid){
        // dont enable reid feature
        return;
    }

    std::vector<Tensor<float>> person_tensor;
    EAGLEEYE_TIME_START(preprocessreid);
    for(int b_i=0; b_i<keeped_num; ++b_i){
        float y1 = classes_bboxes.at(b_i, 0) / o_h;
        float x1 = classes_bboxes.at(b_i, 1) / o_w;
        float y2 = classes_bboxes.at(b_i, 2) / o_h;
        float x2 = classes_bboxes.at(b_i, 3) / o_w;
        float bbox_h = y2 - y1;
        float bbox_w = x2 - x1;
        float roi_level_f = log(sqrt(bbox_h*bbox_w)/(300.0/sqrt(300.0*300.0)))/log(2.0);
        int roi_level = std::min(5, std::max(2, 4+int(round(roi_level_f))));

        int from_level = roi_level - 2;
        Tensor<float> tensor(this->m_output_shapes[10+from_level], EagleeyeRuntime(EAGLEEYE_CPU), (void*)outputs[this->m_output_names[10+from_level]]);
        Tensor<float> bbox_tensor(std::vector<int64_t>{1, 4});
        bbox_tensor.at(0, 0) = y1;
        bbox_tensor.at(0, 1) = x1;
        bbox_tensor.at(0, 2) = y2;
        bbox_tensor.at(0, 3) = x2;
        Tensor<float> crop_feat = crop_and_resize(tensor, bbox_tensor, std::vector<int>{0}, std::vector<int>{7,7});
        person_tensor.push_back(crop_feat);
    }
    EAGLEEYE_TIME_END(preprocessreid);

    Matrix<float> reidfeature;
    if(keeped_num > 0){
        Tensor<float> tesnor_input = concat<float>(person_tensor, 0);
        std::map<std::string, unsigned char*> head_inputs;
        head_inputs["input_reid"] = (unsigned char*)tesnor_input.dataptr();
        std::map<std::string, unsigned char*> head_outputs;
        this->m_reid->setInputShapes(std::vector<std::vector<int64_t>>{tesnor_input.shape()});
        this->m_reid->setOutputShapes(std::vector<std::vector<int64_t>>{{tesnor_input.shape()[0],1,1,256}});

        EAGLEEYE_TIME_START(reidrun);
        bool reid_status = this->m_reid->run(head_inputs, head_outputs);
        EAGLEEYE_TIME_END(reidrun);
        reidfeature = norm(Matrix<float>(tesnor_input.shape()[0],256, head_outputs["person_feature"]), 0);
    }
    output_reidfeature_sig->setData(reidfeature);
}

Matrix<float> SSDReIDPersonDetNode::decodePosition(Matrix<float> prediction_loc, 
                                                    Matrix<float> person_cls,
                                                    int o_h,
                                                    int o_w){
    int rows = prediction_loc.rows();
    Matrix<float> pre_y = prediction_loc(Range(0, rows), Range(0,1));
    Matrix<float> pre_x = prediction_loc(Range(0, rows), Range(1,2));
    Matrix<float> pre_h = prediction_loc(Range(0, rows), Range(2,3));
    Matrix<float> pre_w = prediction_loc(Range(0, rows), Range(3,4));
    
    int anchors_num = this->m_anchors.rows();
    Matrix<float> ycenter_a = this->m_anchors(Range(0, anchors_num),Range(0,1));
    Matrix<float> xcenter_a = this->m_anchors(Range(0, anchors_num),Range(1,2));
    Matrix<float> h_a = this->m_anchors(Range(0, anchors_num),Range(2,3)) - ycenter_a;
    Matrix<float> w_a = this->m_anchors(Range(0, anchors_num),Range(3,4)) - xcenter_a;

    // preson location
    pre_y /= 10.0f;
    pre_x /= 10.0f;
    pre_h /= 5.0f;
    pre_w /= 5.0f;

    Matrix<float> h = mexp(pre_h);
    h = h.mul_(h_a);    // inplace mul
    Matrix<float> w = mexp(pre_w);
    w = w.mul_(w_a);    // inplace mul

    Matrix<float> ycenter = pre_y.mul_(h_a)+ycenter_a;
    Matrix<float> ycenter_cp = ycenter.clone();
    Matrix<float> xcenter = pre_x.mul_(w_a)+xcenter_a;
    Matrix<float> xcenter_cp = xcenter.clone();

    Matrix<float> h_half = h.div_(2.0);
    Matrix<float> w_half = w.div_(2.0);
    // Matrix<float> ymin = (ycenter - h / 2.0) * float(o_h);
    Matrix<float> ymin = ycenter.sub_(h_half).mul_(o_h);
    // Matrix<float> xmin = (xcenter - w / 2.0) * float(o_w);
    Matrix<float> xmin = xcenter.sub_(w_half).mul_(o_w);
    // Matrix<float> ymax = (ycenter + h / 2.0) * float(o_h);
    Matrix<float> ymax = ycenter_cp.add_(h_half).mul_(o_h);
    // Matrix<float> xmax = (xcenter + w / 2.0) * float(o_w);
    Matrix<float> xmax = xcenter_cp.add_(w_half).mul_(o_w);

    Matrix<float> bbox(rows, 5);
    bbox(Range(0, rows), Range(0,1)).copy(ymin);
    bbox(Range(0, rows), Range(1,2)).copy(xmin);
    bbox(Range(0, rows), Range(2,3)).copy(ymax);
    bbox(Range(0, rows), Range(3,4)).copy(xmax);
    bbox(Range(0, rows), Range(4,5)).copy(person_cls);

    return bbox;
}

Matrix<float> SSDReIDPersonDetNode::getAllAnchors() {
	std::vector<std::vector<float>> layers_shapes;
	layers_shapes.push_back(std::vector<float> {38, 38});
	layers_shapes.push_back(std::vector<float> {19, 19});
	layers_shapes.push_back(std::vector<float> {10, 10});
	layers_shapes.push_back(std::vector<float> {5, 5});
	layers_shapes.push_back(std::vector<float> {3, 3});
	// layers_shapes.push_back(std::vector<float> {2, 2});
	// layers_shapes.push_back(std::vector<float> {1, 1});

	std::vector<float> anchor_scales{0.15, 0.3, 0.5, 0.7, 0.9};
	std::vector<float> extra_anchor_scales{0.2, 0.418, 0.67, 0.872, 0.975};
	std::vector<float> layer_steps{8.0, 16.0, 30.0, 64.0, 100.0};

	std::vector<std::vector<float>> anchor_ratios;
	anchor_ratios.push_back(std::vector<float> {1.0, 2.0, 0.5});
	anchor_ratios.push_back(std::vector<float> {1.0, 2.0, 3.0, 0.5, 0.3333});
	anchor_ratios.push_back(std::vector<float> {1.0, 2.0, 3.0, 0.5, 0.3333});
	anchor_ratios.push_back(std::vector<float> {1.0, 2.0, 3.0, 0.5, 0.3333});
	anchor_ratios.push_back(std::vector<float> {1.0, 2.0, 3.0, 0.5, 0.3333});
	// anchor_ratios.push_back(std::vector<float> {1.0, 2.0, 3.0, 0.5, 0.3333});

	std::vector<Matrix<float>> all_layer_anchor_scale_ratio_list;
	for (int i = 0; i < anchor_scales.size(); ++i) {
		Matrix<float> scale_ratio(anchor_ratios[i].size()+1,2);
		scale_ratio.at(0, 0) = extra_anchor_scales[i];
		scale_ratio.at(0, 1) = 1.0;
		for (int c = 0; c < anchor_ratios[i].size(); ++c) {
			scale_ratio.at(c+1, 0) = anchor_scales[i];
			scale_ratio.at(c+1, 1) = anchor_ratios[i][c];
		}

		Matrix<float> bbox = _tile_anchors(layers_shapes[i][0],
													 layers_shapes[i][1],
		                               				 scale_ratio(Range(0, anchor_ratios[i].size() + 1), Range(0, 1)),
		                               				 scale_ratio(Range(0, anchor_ratios[i].size() + 1), Range(1, 2)),
		                               				 std::vector<float> {1.0, 1.0},
		                               				 std::vector<float> {1.0f / (300.0f/layer_steps[i]), 1.0f / (300.0f/layer_steps[i])},
		                               				 std::vector<float> {0.5f / (300.0f/layer_steps[i]), 0.5f / (300.0f/layer_steps[i])});
		all_layer_anchor_scale_ratio_list.push_back(bbox);
	}

	int total_num = 0;
	for (int i = 0; i < all_layer_anchor_scale_ratio_list.size(); ++i) {
		total_num += all_layer_anchor_scale_ratio_list[i].rows();
	}

	Matrix<float> all_anchors(total_num, 4);
	int offset = 0;
	for (int i = 0; i < all_layer_anchor_scale_ratio_list.size(); ++i) {
		unsigned int anchor_rows = all_layer_anchor_scale_ratio_list[i].rows();
		all_anchors(Range(offset, offset + anchor_rows), Range(0, 4)).copy(all_layer_anchor_scale_ratio_list[i]);
		offset += anchor_rows;
	}
	return all_anchors;
}

Matrix<float> SSDReIDPersonDetNode::_tile_anchors(unsigned int grid_height,
                                      unsigned int grid_width,
                                      Matrix<float> scales,
                                      Matrix<float> aspect_ratios,
                                      std::vector<float> base_anchor_size,
                                      std::vector<float> anchor_stride,
                                      std::vector<float> anchor_offset) {
	Matrix<float> ratio_sqrts = msqrt(aspect_ratios);

	Matrix<float> heights = scales.div(ratio_sqrts) * base_anchor_size[0];
	Matrix<float> widths = scales.mul(ratio_sqrts) * base_anchor_size[1];

	Matrix<float> y_centers(1, grid_height);
	for (int i = 0; i < grid_height; ++i) {
		y_centers.at(i) = float(i);
	}
	y_centers = y_centers * anchor_stride[0] + anchor_offset[0];

	Matrix<float> x_centers(1, grid_width);
	for (int i = 0; i < grid_width; ++i) {
		x_centers.at(i) = float(i);
	}
	x_centers = x_centers * anchor_stride[1] + anchor_offset[1];
	Matrix<float> xg;
	Matrix<float> yg;

	meshgrid(x_centers, y_centers, xg, yg);
	Matrix<float> widths_grid;
	Matrix<float> x_centers_grid;

	meshgrid(widths, xg, widths_grid, x_centers_grid);

	Matrix<float> heights_grid;
	Matrix<float> y_centers_grid;

	meshgrid(heights, yg, heights_grid, y_centers_grid);

	int y_rows = y_centers_grid.rows();
	int y_cols = y_centers_grid.cols();
	Matrix<float> bbox_centers(y_rows * y_cols, 4);
	for (int r = 0; r < y_rows; ++r) {
		for (int c = 0; c < y_cols; ++c) {
			bbox_centers.at(r * y_cols + c, 0) = y_centers_grid.at(r, c);
			bbox_centers.at(r * y_cols + c, 1) = x_centers_grid.at(r, c);

			bbox_centers.at(r * y_cols + c, 2) = heights_grid.at(r, c) + y_centers_grid.at(r, c);
			bbox_centers.at(r * y_cols + c, 3) = widths_grid.at(r, c) + x_centers_grid.at(r, c);
		}
	}

	return bbox_centers;
}
}