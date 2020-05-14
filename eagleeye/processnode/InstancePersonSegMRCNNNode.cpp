#include "eagleeye/processnode/InstancePersonSegMRCNNNode.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/basic/MatrixMath.h"
#include "eagleeye/algorithm/nms.h"
#include "eagleeye/tensorop/imageop.h"
#include "eagleeye/tensorop/tensorop.h"
#include "eagleeye/framework/pipeline/AnyMonitor.h"
#include "eagleeye/common/EagleeyeStr.h"

namespace eagleeye
{
InstancePersonSegMRCNNNode::InstancePersonSegMRCNNNode(std::string model_name, std::string device){
    // 1.step basic definition
    // 设置输出端口（拥有2个输出端口）
    this->setNumberOfOutputSignals(2);
    this->setOutputPort(new ImageSignal<int>, 0);
    this->setOutputPort(new TensorSignal<unsigned char>, 1);

    // 设置输入端口（拥有1个输入端口）
    // port 0: frame
	this->setNumberOfInputSignals(1);

    this->m_pre_num_limit = 100;
    // 2.step cnn model definition
    // 1. rpn model
    std::string rpn_model_name = model_name + "_rpn";
    this->m_rpn_input_name = "input_image";

    this->m_rpn_model_h = 512;
    this->m_rpn_model_w = 512;

    std::vector<std::string> rpn_output_list={
        "output_p2",
        "output_p3",
        "output_p4",
        "output_p5",
        "output_p6",
        "output_rpn_class_logits2",
        "output_rpn_class_logits3",
        "output_rpn_class_logits4",
        "output_rpn_class_logits5",
        "output_rpn_class_logits6",
        "output_rpn_bbox2",
        "output_rpn_bbox3",
        "output_rpn_bbox4",
        "output_rpn_bbox5",
        "output_rpn_bbox6"
    };
    std::vector<std::vector<int64_t>> rpn_output_size={
        std::vector<int64_t>{1,128,128,256},
        std::vector<int64_t>{1,64,64,256},
        std::vector<int64_t>{1,32,32,256},
        std::vector<int64_t>{1,16,16,256},
        std::vector<int64_t>{1,8,8,256},
        std::vector<int64_t>{1,128,128,6},
        std::vector<int64_t>{1,64,64,6},
        std::vector<int64_t>{1,32,32,6},
        std::vector<int64_t>{1,16,16,6},
        std::vector<int64_t>{1,8,8,6},
        std::vector<int64_t>{1,128,128,12},
        std::vector<int64_t>{1,64,64,12},
        std::vector<int64_t>{1,32,32,12},
        std::vector<int64_t>{1,16,16,12},
        std::vector<int64_t>{1,8,8,12}
    };
    
    this->m_rpn_model = new ModelRun(rpn_model_name, 
                                     device, 
                                     std::vector<std::string>{m_rpn_input_name},
                                     std::vector<std::vector<int64_t>>{std::vector<int64_t>{1,m_rpn_model_h,m_rpn_model_w,3}},
                                     rpn_output_list,
                                     rpn_output_size);

    std::map<std::string, std::string> output_name_map_for_rpn;
    output_name_map_for_rpn["output_rpn_class_logits2"] = "rpn_model/rpn_class_raw/convolution";
    output_name_map_for_rpn["output_rpn_class_logits3"] = "rpn_model_1/rpn_class_raw/convolution";
    output_name_map_for_rpn["output_rpn_class_logits4"] = "rpn_model_2/rpn_class_raw/convolution";
    output_name_map_for_rpn["output_rpn_class_logits5"] = "rpn_model_3/rpn_class_raw/convolution";
    output_name_map_for_rpn["output_rpn_class_logits6"] = "rpn_model_4/rpn_class_raw/convolution";

    output_name_map_for_rpn["output_rpn_bbox2"] = "rpn_model/rpn_bbox_pred/convolution";
    output_name_map_for_rpn["output_rpn_bbox3"] = "rpn_model_1/rpn_bbox_pred/convolution";
    output_name_map_for_rpn["output_rpn_bbox4"] = "rpn_model_2/rpn_bbox_pred/convolution";
    output_name_map_for_rpn["output_rpn_bbox5"] = "rpn_model_3/rpn_bbox_pred/convolution";
    output_name_map_for_rpn["output_rpn_bbox6"] = "rpn_model_4/rpn_bbox_pred/convolution";

    output_name_map_for_rpn["output_p2"] = "fpn_p2/convolution";
    output_name_map_for_rpn["output_p3"] = "fpn_p3/convolution";
    output_name_map_for_rpn["output_p4"] = "fpn_p4/convolution";
    output_name_map_for_rpn["output_p5"] = "fpn_p5/convolution";
    output_name_map_for_rpn["output_p6"] = "fpn_p6/MaxPool";
    this->m_rpn_model->setOutputNameMap(output_name_map_for_rpn);

    std::map<std::string, std::string> output_name_map2_for_rpn;
    output_name_map2_for_rpn["output_rpn_class_logits2"] = "rpn_model/rpn_class_raw/BiasAdd";
    output_name_map2_for_rpn["output_rpn_class_logits3"] = "rpn_model_1/rpn_class_raw/BiasAdd";
    output_name_map2_for_rpn["output_rpn_class_logits4"] = "rpn_model_2/rpn_class_raw/BiasAdd";
    output_name_map2_for_rpn["output_rpn_class_logits5"] = "rpn_model_3/rpn_class_raw/BiasAdd";
    output_name_map2_for_rpn["output_rpn_class_logits6"] = "rpn_model_4/rpn_class_raw/BiasAdd";

    output_name_map2_for_rpn["output_rpn_bbox2"] = "rpn_model/rpn_bbox_pred/BiasAdd";
    output_name_map2_for_rpn["output_rpn_bbox3"] = "rpn_model_1/rpn_bbox_pred/BiasAdd";
    output_name_map2_for_rpn["output_rpn_bbox4"] = "rpn_model_2/rpn_bbox_pred/BiasAdd";
    output_name_map2_for_rpn["output_rpn_bbox5"] = "rpn_model_3/rpn_bbox_pred/BiasAdd";
    output_name_map2_for_rpn["output_rpn_bbox6"] = "rpn_model_4/rpn_bbox_pred/BiasAdd";

    output_name_map2_for_rpn["output_p2"] = "fpn_p2/BiasAdd";
    output_name_map2_for_rpn["output_p3"] = "fpn_p3/BiasAdd";
    output_name_map2_for_rpn["output_p4"] = "fpn_p4/BiasAdd";
    output_name_map2_for_rpn["output_p5"] = "fpn_p5/BiasAdd";
    output_name_map2_for_rpn["output_p6"] = "fpn_p6/MaxPool";
    this->m_rpn_model->setOutputNameMap2(output_name_map2_for_rpn);

    // 2. detect model
    std::string det_model_name = model_name + "_det";
    this->m_det_model = new ModelRun(det_model_name,
                                     device,
                                     std::vector<std::string>{"rois_in"},
                                     std::vector<std::vector<int64_t>>{
                                         std::vector<int64_t>{-1,7,7,256}
                                     },
                                     std::vector<std::string>{"output_mrcnn_class_logits",
                                                              "output_mrcnn_bbox"
                                     },
                                     std::vector<std::vector<int64_t>>{
                                         std::vector<int64_t>{-1,1,1,2},
                                         std::vector<int64_t>{-1,1,1,2*4}
                                     });
    std::map<std::string, std::string> output_name_map_for_det;
    output_name_map_for_det["output_mrcnn_class_logits"] = "mrcnn_class_logits/convolution";
    output_name_map_for_det["output_mrcnn_bbox"] = "mrcnn_bbox_fc/convolution";
    this->m_det_model->setOutputNameMap(output_name_map_for_det);

    std::map<std::string, std::string> output_name_map2_for_det;
    output_name_map2_for_det["output_mrcnn_class_logits"] = "mrcnn_class_logits/BiasAdd";
    output_name_map2_for_det["output_mrcnn_bbox"] = "mrcnn_bbox_fc/BiasAdd";
    this->m_det_model->setOutputNameMap2(output_name_map2_for_det);

    // 3. mask model
    std::string mask_model_name = model_name + "_mask";
    this->m_mask_model = new ModelRun(mask_model_name,
                                      device,
                                      std::vector<std::string>{"roismask_in"},
                                      std::vector<std::vector<int64_t>>{
                                          std::vector<int64_t>{-1, 14, 14, 256}
                                      },
                                      std::vector<std::string>{"output_mrcnn_mask"},
                                      std::vector<std::vector<int64_t>>{
                                          std::vector<int64_t>{-1, 28, 28, 2}
                                      });

    std::map<std::string, std::string> output_name_map_for_mask;
    output_name_map_for_mask["output_mrcnn_mask"] = "mrcnn_mask/Sigmoid";
    this->m_mask_model->setOutputNameMap(output_name_map_for_mask);

    std::map<std::string, std::string> output_name_map2_for_mask;
    output_name_map2_for_mask["output_mrcnn_mask"] = "mrcnn_mask/Sigmoid";
    this->m_mask_model->setOutputNameMap2(output_name_map2_for_mask);

    // get anchors
    this->m_anchors = this->getAnchors(512,512,
                            std::vector<float>{16.0f, 32.0f, 64.0f, 128.0f, 256.0f},
                            std::vector<float>{0.5, 1, 2},
                            std::vector<std::vector<int>>{
                                std::vector<int>{128,128},
                                std::vector<int>{64,64},
                                std::vector<int>{32,32},
                                std::vector<int>{16,16},
                                std::vector<int>{8,8}
                            },
                            std::vector<int>{4,8,16,32,64},
                            1);
    
    this->m_nms_rpn_thres = 0.3;
    this->m_nms_refine_thres = 0.9;
    this->m_seg_thres = 0.2;

    // add monitor variable
    EAGLEEYE_MONITOR_VAR(int, setMaxPersonNum, getMaxPersonNum, "maxpersonnum","1","20");
    EAGLEEYE_MONITOR_VAR(std::string, setModelPath, getModelPath,"modelpath", "", "");
    EAGLEEYE_MONITOR_VAR(float, setRPNNMSThreashold, getRPNNMSThreashold, "rpn_nms_thres","0.5","1.0");
    EAGLEEYE_MONITOR_VAR(float, setRefinedNMSThreashold, getRefinedNMSThreashold, "refie_nms_thres","0.5","1.0");
    EAGLEEYE_MONITOR_VAR(float, setSegThres, getSegThres, "seg_thres", "0.2", "1.0");
}   

InstancePersonSegMRCNNNode::~InstancePersonSegMRCNNNode(){
    if(this->m_rpn_model != NULL){
        delete this->m_rpn_model;
    }
    
    if(this->m_det_model != NULL){
        delete this->m_det_model;
    }
    
    if(this->m_mask_model != NULL){
        delete this->m_mask_model;
    }
} 

void InstancePersonSegMRCNNNode::executeNodeInfo(){
    // 1.step get image from input signal
    ImageSignal<Array<unsigned char, 3>>* frame_sig = (ImageSignal<Array<unsigned char, 3>>*)(this->m_input_signals[0]);
    Matrix<Array<unsigned char, 3>> frame = frame_sig->getData();
    int frame_height = frame.rows();
    int frame_width = frame.cols();

    // 2.step preprocess input
    Matrix<Array<unsigned char,3>> output_image;
    Matrix<int> window;
    float scale;
    Matrix<int> padding;
    Matrix<int> crop;
    moldInput(frame, output_image, 0, 512, window, scale, padding, crop);

    float mean[3] = {123.7f, 116.8f, 103.9f};
    float var[3] = {1.0f,1.0f,1.0f};
    Matrix<Array<float,3>> preprocesed_input = moldImage(output_image, mean, var);

    // 3.step get itermediate feature map from rpn model
    EAGLEEYE_LOGD("MRCNN FPN and RPN STAGE");
    std::map<std::string, unsigned char*> rpn_inputs;
    rpn_inputs[m_rpn_input_name] = (unsigned char*)preprocesed_input.dataptr();
    std::map<std::string, unsigned char*> rpn_outputs;
    EAGLEEYE_TIME_START(rpnmodel);
    bool status = this->m_rpn_model->run(rpn_inputs, rpn_outputs);
    EAGLEEYE_TIME_END(rpnmodel);    
    Tensor<float> p2(std::vector<int64_t>{1,128,128,256},rpn_outputs["output_p2"], false);
    Tensor<float> p3(std::vector<int64_t>{1,64,64,256}, rpn_outputs["output_p3"], false);
    Tensor<float> p4(std::vector<int64_t>{1,32,32,256}, rpn_outputs["output_p4"], false);
    Tensor<float> p5(std::vector<int64_t>{1,16,16,256}, rpn_outputs["output_p5"], false);

    Matrix<float> rpn_logits_from_p2(128*128*3,2, rpn_outputs["output_rpn_class_logits2"], false);
    Matrix<float> rpn_logits_from_p3(64*64*3,2, rpn_outputs["output_rpn_class_logits3"], false);
    Matrix<float> rpn_logits_from_p4(32*32*3,2, rpn_outputs["output_rpn_class_logits4"], false);
    Matrix<float> rpn_logits_from_p5(16*16*3,2, rpn_outputs["output_rpn_class_logits5"], false);
    Matrix<float> rpn_logits_from_p6(8*8*3,2, rpn_outputs["output_rpn_class_logits6"], false);

    Matrix<float> rpn_bbox_from_p2(128*128*3,4,rpn_outputs["output_rpn_bbox2"], false);
    Matrix<float> rpn_bbox_from_p3(64*64*3,4,rpn_outputs["output_rpn_bbox3"], false);
    Matrix<float> rpn_bbox_from_p4(32*32*3,4,rpn_outputs["output_rpn_bbox4"], false);
    Matrix<float> rpn_bbox_from_p5(16*16*3,4,rpn_outputs["output_rpn_bbox5"], false);
    Matrix<float> rpn_bbox_from_p6(8*8*3,4,rpn_outputs["output_rpn_bbox6"], false);
    
    std::vector<Matrix<float>> rpn_logits_list=
                    std::vector<Matrix<float>>{rpn_logits_from_p2,
                                               rpn_logits_from_p3,
                                               rpn_logits_from_p4,
                                               rpn_logits_from_p5,
                                               rpn_logits_from_p6};
    Matrix<float> rpn_logits = concat(rpn_logits_list, 0);
    Matrix<float> rpn_probs = msoftmax(rpn_logits);

    std::vector<Matrix<float>> rpn_bbox_list=
                    std::vector<Matrix<float>>{rpn_bbox_from_p2,
                                               rpn_bbox_from_p3,
                                               rpn_bbox_from_p4,
                                               rpn_bbox_from_p5,
                                               rpn_bbox_from_p6};
    Matrix<float> rpn_bboxs = concat(rpn_bbox_list, 0);

    // 4.step get proposal from det model
    // N x 4
    // y1,x1,y2,x2 (归一化检测框)
    Matrix<float> rpn_rois = this->getProposals(rpn_probs, rpn_bboxs, this->m_anchors);
    // // /////////////////////////////////////
    // rpn_rois = Matrix<float>(100,4);
    // FILE* fp = fopen("/data/local/tmp/GGT/rpn_rois.bin","rb");
    // int read_size = rpn_rois.rows()*rpn_rois.cols();
    // fread(rpn_rois.dataptr(),sizeof(float),read_size,fp);
    // fclose(fp);
    // std::cout<<"show preprocessed input step 2"<<std::endl;
    // for(int i=0; i<10; ++i){
    //     std::cout<<rpn_rois.at(i,0)<<" "<<rpn_rois.at(i,1)<<" "<<rpn_rois.at(i,2)<<" "<<rpn_rois.at(i,3)<<std::endl;
    // }
    // std::cout<<"\n\n";
    // // /////////////////////////////////////

    int rpn_rois_num = rpn_rois.rows();
    if(rpn_rois_num == 0){
        ImageSignal<int>* output_roi_sig = (ImageSignal<int>*)(this->getOutputPort(0));
        TensorSignal<unsigned char>* output_mask_sig = (TensorSignal<unsigned char>*)(this->getOutputPort(1));
        output_roi_sig->setData(Matrix<int>());
        output_mask_sig->setData(Tensor<unsigned char>());
        return;
    }
    EAGLEEYE_LOGD("RPN Get %d Objects", rpn_rois_num);

    // N, 7, 7, 256
    Tensor<float> rpn_proposals_roialign = this->roiAlign(rpn_rois, 
                                                          std::vector<Tensor<float>>{p2,p3,p4,p5},
                                                          7);    

    EAGLEEYE_LOGD("MRCNN REFINE DET STAGE");
    std::map<std::string, unsigned char*> proposal_inputs;
    proposal_inputs["rois_in"] = (unsigned char*)rpn_proposals_roialign.dataptr();
    std::map<std::string, unsigned char*> proposal_outputs;
    this->m_det_model->setInputShapes(std::vector<std::vector<int64_t>>{std::vector<int64_t>{rpn_rois_num,7,7,256}});
    this->m_det_model->setOutputShapes(std::vector<std::vector<int64_t>>{
                                            std::vector<int64_t>{rpn_rois_num,1,1,2},
                                            std::vector<int64_t>{rpn_rois_num,1,1,8}});

    EAGLEEYE_TIME_START(proposalmodel);
    status = this->m_det_model->run(proposal_inputs, proposal_outputs);
    EAGLEEYE_TIME_END(proposalmodel);    
    Matrix<float> mrcnn_refined_logits(rpn_rois_num,2,proposal_outputs["output_mrcnn_class_logits"],false);
    Matrix<float> mrcnn_refined_bboxs(rpn_rois_num,2*4,proposal_outputs["output_mrcnn_bbox"],false);
    Matrix<float> mrcnn_refined_probs = msoftmax(mrcnn_refined_logits);

    // // /////////////////////////////////////
    // mrcnn_refined_bboxs = Matrix<float>(100,8);
    // FILE* bboxs_fp = fopen("/data/local/tmp/GGT/mrcnn_bbox.bin","rb");
    // int bboxs_fp_read_size = mrcnn_refined_bboxs.rows()*mrcnn_refined_bboxs.cols();
    // fread(mrcnn_refined_bboxs.dataptr(),sizeof(float),bboxs_fp_read_size,bboxs_fp);
    // fclose(bboxs_fp);
    // std::cout<<"refined bboxes"<<std::endl;
    // std::cout<<mrcnn_refined_bboxs(Range(0,2),Range(0,8));

    // mrcnn_refined_probs = Matrix<float>(100,2);
    // FILE* probs_fp = fopen("/data/local/tmp/GGT/mrcnn_class.bin","rb");
    // int probs_fp_read_size = mrcnn_refined_probs.rows()*mrcnn_refined_probs.cols();
    // fread(mrcnn_refined_probs.dataptr(),sizeof(float),probs_fp_read_size,probs_fp);
    // fclose(probs_fp);

    // std::cout<<"refined probs"<<std::endl;
    // std::cout<<mrcnn_refined_probs(Range(0,2),Range(0,2));
    // // /////////////////////////////////////

    // N x 4
    // y1,x1,y2,x2 (归一化检测框)
    Matrix<float> mrcnn_rois = this->getRefineProposals(rpn_rois, mrcnn_refined_bboxs, mrcnn_refined_probs);
    EAGLEEYE_LOGD("REFINE Get %d Objects", mrcnn_rois.rows());

    // // /////////////////////////////////////
    // mrcnn_rois = Matrix<float>(100,4);
    // FILE* fp = fopen("/data/local/tmp/GGT/detection_output.bin","rb");
    // int read_size = mrcnn_rois.rows()*mrcnn_rois.cols();
    // fread(mrcnn_rois.dataptr(),sizeof(float),read_size,fp);
    // fclose(fp);
    // std::cout<<"show preprocessed input step 2"<<std::endl;
    // for(int i=0; i<10; ++i){
    //     std::cout<<mrcnn_rois.at(i,0)<<" "<<mrcnn_rois.at(i,1)<<" "<<mrcnn_rois.at(i,2)<<" "<<mrcnn_rois.at(i,3)<<std::endl;
    // }
    // std::cout<<"\n\n";
    // mrcnn_rois = mrcnn_rois(Range(0,6),Range(0,4));

    // // /////////////////////////////////////
    

    // 5.step get proposal from mask model
    // 100, 14, 14, 256
    EAGLEEYE_LOGD("MRCNN MASK STAGE");
    int refined_rois_num = mrcnn_rois.rows();
    if(refined_rois_num == 0){
        ImageSignal<int>* output_roi_sig = (ImageSignal<int>*)(this->getOutputPort(0));
        TensorSignal<unsigned char>* output_mask_sig = (TensorSignal<unsigned char>*)(this->getOutputPort(1));
        output_roi_sig->setData(Matrix<int>());
        output_mask_sig->setData(Tensor<unsigned char>());
        return;
    }

    Tensor<float> mask_roialign = this->roiAlign(mrcnn_rois, 
                                                 std::vector<Tensor<float>>{p2,p3,p4,p5},
                                                 14);

    this->m_mask_model->setInputShapes(std::vector<std::vector<int64_t>>{
                                            std::vector<int64_t>{refined_rois_num,14,14,256}});
    this->m_mask_model->setOutputShapes(std::vector<std::vector<int64_t>>{
                                            std::vector<int64_t>{refined_rois_num,28,28,2}});
    
    std::map<std::string, unsigned char*> mask_inputs;
    mask_inputs["roismask_in"] = (unsigned char*)mask_roialign.dataptr();
    std::map<std::string, unsigned char*> mask_outputs;
    EAGLEEYE_TIME_START(maskmodel);
    status = this->m_mask_model->run(mask_inputs, mask_outputs);
    EAGLEEYE_TIME_END(maskmodel);    

    // 100,28,28,2
    Tensor<float> mrcnn_masks(std::vector<int64_t>{refined_rois_num,28,28,2},
                          mask_outputs["output_mrcnn_mask"],
                          false);

    Matrix<int> final_rois;               // (N,4)
    Tensor<unsigned char> final_mask;     // H,W
    unmoldMRCNN(mrcnn_rois, mrcnn_masks, frame_height, frame_width, 512, 512, window, final_rois, final_mask);

    // 输出区域和分割图到输出信号
    ImageSignal<int>* output_roi_sig = (ImageSignal<int>*)(this->getOutputPort(0));
    TensorSignal<unsigned char>* output_mask_sig = (TensorSignal<unsigned char>*)(this->getOutputPort(1));
    output_roi_sig->setData(final_rois);
    output_mask_sig->setData(final_mask);
}

Matrix<float> InstancePersonSegMRCNNNode::getProposals(Matrix<float> rpn_probs,
                                                       Matrix<float> rpn_bboxs, 
                                                       Matrix<float> anchors){
    // rpn_probs (65472, 2)
    // rpn_bboxs (65472, 4)
    // anchors (65472, 4)
    int rpn_rows = rpn_probs.rows();

    Matrix<float> RPN_BBOX_STD_DEV(1,4);
    RPN_BBOX_STD_DEV.at(0,0) = 0.1f; RPN_BBOX_STD_DEV.at(0,1) = 0.1f; RPN_BBOX_STD_DEV.at(0,2) = 0.2f; RPN_BBOX_STD_DEV.at(0,3) = 0.2f;
    
    Matrix<float> deltas = rpn_bboxs.clone();
    deltas.mul_(RPN_BBOX_STD_DEV);

    int nms_limit = eagleeye_min(this->m_pre_num_limit, anchors.rows());    
    Matrix<float> rpn_probs_p = rpn_probs(Range(0, rpn_rows), Range(1,2));
    std::vector<unsigned int> idx = sort<DescendingSort<float>>(rpn_probs_p);
    std::vector<unsigned int> top_idx;
    for(int i=0; i<nms_limit; ++i){
        top_idx.push_back(idx[i]);
    }

    Matrix<float> topk_scores = rpn_probs_p.select(top_idx);
    Matrix<float> topk_deltas = deltas.select(top_idx);
    Matrix<float> topk_anchors = anchors.select(top_idx);

    /****************  restore bbox ********************/
    Matrix<float> boxes_height = topk_anchors(Range(0, nms_limit), Range(2,3)) - topk_anchors(Range(0,nms_limit),Range(0,1));
    Matrix<float> boxes_width = topk_anchors(Range(0,nms_limit),Range(3,4)) - topk_anchors(Range(0,nms_limit),Range(1,2));
    Matrix<float> center_y = topk_anchors(Range(0,nms_limit),Range(0,1)) + boxes_height * 0.5f;
    Matrix<float> center_x = topk_anchors(Range(0,nms_limit), Range(1,2)) + boxes_width * 0.5f;

    // apply deltas
    center_y += topk_deltas(Range(0,nms_limit),Range(0,1)).mul(boxes_height);
    center_x += topk_deltas(Range(0,nms_limit),Range(1,2)).mul(boxes_width);
    boxes_height.mul_(mexp(topk_deltas(Range(0,nms_limit),Range(2,3))));
    boxes_width.mul_(mexp(topk_deltas(Range(0,nms_limit),Range(3,4))));

    // conver back to y1,x1,y2,x2
    Matrix<float> y1 = center_y - boxes_height * 0.5f;
    Matrix<float> x1 = center_x - boxes_width * 0.5f;
    Matrix<float> y2 = y1 + boxes_height;
    Matrix<float> x2 = x1 + boxes_width;

    std::vector<Matrix<float>> proposals_list={y1,x1,y2,x2};
    Matrix<float> proposals = concat(proposals_list, 1);

    /* clip by window (0,0,1,1)*/
    for(int i=0; i<nms_limit; ++i){
        float* ptr = proposals.row(i);
        ptr[0] = eagleeye_clip(ptr[0], 0.0f, 1.0f);
        ptr[1] = eagleeye_clip(ptr[1], 0.0f, 1.0f);
        ptr[2] = eagleeye_clip(ptr[2], 0.0f, 1.0f);
        ptr[3] = eagleeye_clip(ptr[3], 0.0f, 1.0f);
    }

    /* nms */
    std::vector<Matrix<float>> proposals_and_scores_list={proposals, topk_scores};
    Matrix<float> proposals_and_scores = concat(proposals_and_scores_list, 1);
    std::vector<unsigned int> nms_idxs = nms(proposals_and_scores, 0.7, this->m_nms_rpn_thres);

    Matrix<float> nms_proposals = proposals.select(nms_idxs);
    return nms_proposals;
}

Tensor<float> InstancePersonSegMRCNNNode::roiAlign(Matrix<float> rpn_rois, 
                                                   std::vector<Tensor<float>> mrcnn_feature_maps, 
                                                   int pool_size){
    std::vector<Matrix<float>> y1x1y2x2 = split(rpn_rois, 4, 1);
    Matrix<float> y1 = y1x1y2x2[0];
    Matrix<float> x1 = y1x1y2x2[1];
    Matrix<float> y2 = y1x1y2x2[2];
    Matrix<float> x2 = y1x1y2x2[3];

    Matrix<float> h = y2-y1;
    Matrix<float> w = x2-x1;

    int num = rpn_rois.rows();
    std::vector<Tensor<float>> crop_feat_list;
    for(int b_i=0; b_i<num; ++b_i){
        float roi_level_f = log(sqrt(h.at(b_i)*w.at(b_i))/(224.0/sqrt(512.0*512.0)))/log(2.0);
        int roi_level = std::min(5, std::max(2, 4+int(round(roi_level_f))));
        
        int feature_map_level = roi_level - 2;

        Tensor<float> bbox_tensor(std::vector<int64_t>{1, 4});
        bbox_tensor.at(0, 0) = y1.at(b_i);
        bbox_tensor.at(0, 1) = x1.at(b_i);
        bbox_tensor.at(0, 2) = y2.at(b_i);
        bbox_tensor.at(0, 3) = x2.at(b_i);
        Tensor<float> crop_feat = crop_and_resize(mrcnn_feature_maps[feature_map_level], bbox_tensor, std::vector<int>{0}, std::vector<int>{pool_size, pool_size});
        crop_feat_list.push_back(crop_feat);
    }   

    // Pack pooled features into one tensor
    Tensor<float> crop_feats = concat<float>(crop_feat_list, 0);
    return crop_feats;
}

Matrix<float> InstancePersonSegMRCNNNode::getRefineProposals(Matrix<float> rpn_rois, 
                                                             Matrix<float> refined_deltas,
                                                             Matrix<float> refined_probs){
    // [rois, mrcnn_class, mrcnn_bbox, window]
    int num = refined_probs.rows();
    Matrix<int> class_ids = argmax(refined_probs, 1);
    Matrix<int> indices = arange<int>(0, num, 1, 0);
    std::vector<Matrix<int>> indices_list={indices, class_ids};
    indices = concat(indices_list, 1);    

    Matrix<float> class_scores = refined_probs.gatherND(indices, 1);
    Matrix<float> class_deltas = refined_deltas.gatherND(indices, 4);

    Matrix<float> RPN_BBOX_STD_DEV(1,4);
    RPN_BBOX_STD_DEV.at(0,0) = 0.1f; RPN_BBOX_STD_DEV.at(0,1) = 0.1f; RPN_BBOX_STD_DEV.at(0,2) = 0.2f; RPN_BBOX_STD_DEV.at(0,3) = 0.2f;
    class_deltas.mul_(RPN_BBOX_STD_DEV);
    
    /****************  restore bbox ********************/
    Matrix<float> boxes_height = rpn_rois(Range(0, num), Range(2,3)) - rpn_rois(Range(0,num),Range(0,1));
    Matrix<float> boxes_width = rpn_rois(Range(0,num),Range(3,4)) - rpn_rois(Range(0,num),Range(1,2));
    Matrix<float> center_y = rpn_rois(Range(0,num),Range(0,1)) + boxes_height * 0.5f;
    Matrix<float> center_x = rpn_rois(Range(0,num), Range(1,2)) + boxes_width * 0.5f;

    // apply deltas
    center_y += class_deltas(Range(0,num),Range(0,1)).mul(boxes_height);
    center_x += class_deltas(Range(0,num),Range(1,2)).mul(boxes_width);
    boxes_height.mul_(mexp(class_deltas(Range(0,num),Range(2,3))));
    boxes_width.mul_(mexp(class_deltas(Range(0,num),Range(3,4))));

    // conver back to y1,x1,y2,x2
    Matrix<float> y1 = center_y - boxes_height * 0.5f;
    Matrix<float> x1 = center_x - boxes_width * 0.5f;
    Matrix<float> y2 = y1 + boxes_height;
    Matrix<float> x2 = x1 + boxes_width;

    std::vector<Matrix<float>> proposals_list={y1,x1,y2,x2};
    Matrix<float> proposals = concat(proposals_list, 1);
    
    /* clip by window (0,0,1,1)*/
    for(int i=0; i<num; ++i){
        float* ptr = proposals.row(i);
        ptr[0] = eagleeye_clip(ptr[0], 0.0f, 1.0f);
        ptr[1] = eagleeye_clip(ptr[1], 0.0f, 1.0f);
        ptr[2] = eagleeye_clip(ptr[2], 0.0f, 1.0f);
        ptr[3] = eagleeye_clip(ptr[3], 0.0f, 1.0f);
    }
    
    /* Filter out background boxes */
    Matrix<int> keep = where<ggt<int>>(class_ids, 0);
    keep = keep(Range(0, keep.rows()), Range(0,1));
    int keep_num = keep.rows();

    Matrix<int> conf_keep = where<gt<float>>(class_scores, 0.7f);
    conf_keep = conf_keep(Range(0, conf_keep.rows()), Range(0,1));
    int conf_keep_num = conf_keep.rows();

    bool* keep_flag = (bool*)malloc(sizeof(bool)*num);
    bool* conf_keep_flag = (bool*)malloc(sizeof(bool)*num);
    memset(keep_flag,0,sizeof(bool)*num);
    memset(conf_keep_flag,0,sizeof(bool)*num);

    for(int i=0; i<keep_num; ++i){
        keep_flag[keep.at(i)] = true;
    }
    for(int i=0; i<conf_keep_num; ++i){
        conf_keep_flag[conf_keep.at(i)] = true;
    }

    int filter_out_num = 0;
    std::vector<int> filter_index;
    for(int i=0; i<num; ++i){
        if(keep_flag[i] && conf_keep_flag[i]){
            filter_index.push_back(i);
        }
    }
    free(keep_flag);
    free(conf_keep_flag);

    Matrix<int> filter_keep(filter_index.size(), 1);
    for(int i=0; i<filter_index.size(); ++i){
        filter_keep.at(i) = filter_index[i];
    }

    // N x 1
    Matrix<int> filter_class_ids = class_ids.gather(filter_keep, 0);
    // N x 1
    Matrix<float> filter_class_scores = class_scores.gather(filter_keep, 0);
    // N x 4
    Matrix<float> filter_proposals = proposals.gather(filter_keep, 0);

    /* NMS */
    std::vector<Matrix<float>> filter_ps_list={filter_proposals, filter_class_scores};
    Matrix<float> filter_ps = concat(filter_ps_list, 1);
    std::vector<unsigned int> nms_idxs = nms(filter_ps, 0.3, this->m_nms_refine_thres);
    Matrix<float> nms_filter_proposals = filter_proposals.select(nms_idxs);
    
    return nms_filter_proposals;
}

void InstancePersonSegMRCNNNode::unmoldMRCNN(Matrix<float> mrcnn_rois, 
                 Tensor<float> mrcnn_masks, 
                 int original_height, 
                 int original_width, 
                 int model_height, 
                 int model_width, 
                 Matrix<int> window, 
                 Matrix<int>& final_rois, 
                 Tensor<unsigned char>& final_masks){
    // mrcnn_rois N x 4
    // mrcnn_mask N x 28 x 28 x 2
    int num = mrcnn_rois.rows();

    // conver normlized bbox to original space
    // get normlized window
    Matrix<float> norm_window = this->normBoxes(window, model_height, model_width);

    // Convert boxes to normalized coordinates on the window
    float y1 = norm_window.at(0,0);
    float x1 = norm_window.at(0,1);
    float y2 = norm_window.at(0,2);
    float x2 = norm_window.at(0,3);

    Matrix<float> mrcnn_rois_based_window = mrcnn_rois.clone();
    float offset_y = y1;
    float offset_x = x1;
    float scale_h = y2 - y1;
    float scale_w = x2 - x1;
    for(int i=0; i<num; ++i){
        float* ptr = mrcnn_rois_based_window.row(i);
        ptr[0] = (ptr[0] - offset_y)/scale_h;
        ptr[1] = (ptr[1] - offset_x)/scale_w;
        ptr[2] = (ptr[2] - offset_y)/scale_h;
        ptr[3] = (ptr[3] - offset_x)/scale_w;
    }
    
    // final rois (in original size)
    final_rois = this->denormBoxes(mrcnn_rois_based_window, original_height, original_width);

    // apply output mask space
    final_masks = Tensor<unsigned char>(std::vector<int64_t>{num, original_height, original_width, 1});

    // crop mask from model output
    std::vector<int64_t> shape = mrcnn_masks.shape();
    float* mrcnn_masks_ptr = mrcnn_masks.dataptr();

    for(int i=0; i<num; ++i){
        int slice_offset = i * shape[1]*shape[2]*shape[3];
        Matrix<float> obj_mask(shape[1], shape[2]);
        for(int r=0; r<shape[1]; ++r){
            float* obj_mask_ptr = obj_mask.row(r);
            for(int c=0; c<shape[2]; ++c){
                int offset = slice_offset + (r*shape[2] + c)*2;
                obj_mask_ptr[c] = *(mrcnn_masks_ptr + offset + 1);
            }
        } 

        // resize to object region
        int obj_y1 = final_rois.at(i,0); int obj_x1 = final_rois.at(i,1); int obj_y2 = final_rois.at(i,2); int obj_x2 = final_rois.at(i,3);
        Matrix<float> resized_obj_mask = resize(obj_mask, (obj_y2 - obj_y1), (obj_x2 - obj_x1), BILINEAR_INTERPOLATION);

        // copy to final masks
        Matrix<unsigned char> resized_obj_binary_mask = boolean<gt<float>, unsigned char>(resized_obj_mask, this->m_seg_thres);
        Matrix<unsigned char> final_masks_slice(original_height, original_width, final_masks.dataptr() + i * original_height * original_width);
        final_masks_slice(Range(obj_y1,obj_y2),Range(obj_x1,obj_x2)).copy(resized_obj_binary_mask);
    }
}

void InstancePersonSegMRCNNNode::setModelPath(std::string path){
    EAGLEEYE_LOGD("initialize MASKRCNN model path %s", path.c_str());
    this->m_rpn_model->setWritablePath(path);
    this->m_rpn_model->initialize();

    this->m_det_model->setWritablePath(path);
    this->m_det_model->initialize();

    this->m_mask_model->setWritablePath(path);
    this->m_mask_model->initialize();
}

void InstancePersonSegMRCNNNode::getModelPath(std::string& path){
    path = this->m_rpn_model->getWritablePath();
}

void InstancePersonSegMRCNNNode::setMaxPersonNum(int max_person_num){
    this->m_pre_num_limit = max_person_num;
}
void InstancePersonSegMRCNNNode::getMaxPersonNum(int& max_person_num){
    max_person_num = this->m_pre_num_limit;
}

void InstancePersonSegMRCNNNode::setRPNNMSThreashold(float thres){
    this->m_nms_rpn_thres = thres;
}
void InstancePersonSegMRCNNNode::getRPNNMSThreashold(float& thres){
    thres = this->m_nms_rpn_thres;
}

void InstancePersonSegMRCNNNode::setRefinedNMSThreashold(float thres){
    this->m_nms_refine_thres = thres;
}
void InstancePersonSegMRCNNNode::getRefinedNMSThreashold(float& thres){
    thres = this->m_nms_refine_thres;
}

void InstancePersonSegMRCNNNode::setSegThres(float thres){
    this->m_seg_thres = thres;
}
void InstancePersonSegMRCNNNode::getSegThres(float& thres){
    thres = this->m_seg_thres;
}
} // namespace eagleeye
