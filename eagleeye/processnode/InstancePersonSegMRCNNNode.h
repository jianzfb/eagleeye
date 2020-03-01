#ifndef _EAGLEEYE_INSTANCEPERSONSEGMRCNNNODE_H_
#define _EAGLEEYE_INSTANCEPERSONSEGMRCNNNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/processnode/NNNode.h"
#include "eagleeye/engine/proxy_run.h"
#include "eagleeye/basic/Tensor.h"
namespace eagleeye{

class InstancePersonSegMRCNNNode:public NNNode{
public:
    typedef InstancePersonSegMRCNNNode          Self;
    typedef NNNode                              Superclass;
    
    InstancePersonSegMRCNNNode(std::string model_name, std::string device);
    virtual ~InstancePersonSegMRCNNNode();

    /**
     *	@brief get class identity
     */
    EAGLEEYE_CLASSIDENTITY(InstancePersonSegMRCNNNode);

    /**
     *	@brief execute MRCNN algorithm
     *  @note user must finish this function
     */
    virtual void executeNodeInfo();
    
    /**
     * @brief Set/Get the Model Path object
     * 
     * @param path 
     */
    void setModelPath(std::string path);
    void getModelPath(std::string& path);

    /**
     * @brief Set the Max Person Num object
     * 
     * @param max_person_num 
     */
    void setMaxPersonNum(int max_person_num);
    void getMaxPersonNum(int& max_person_num);

    /**
     * @brief set RPN nms threshold
     * 
     * @param thres 
     */
    void setRPNNMSThreashold(float thres);
    void getRPNNMSThreashold(float& thres);

    /**
     * @brief Set/Get nms thres in refine stage
     * 
     * @param thres 
     */
    void setRefinedNMSThreashold(float thres);
    void getRefinedNMSThreashold(float& thres);

    /**
     * @brief Set/Get the Seg Thres object
     * 
     * @param thres 
     */
    void setSegThres(float thres);
    void getSegThres(float& thres);

private:
    InstancePersonSegMRCNNNode(const InstancePersonSegMRCNNNode&);
    void operator=(const InstancePersonSegMRCNNNode&); 

    /**
     * @brief Get the Proposals object
     * 
     * @param rpn_probs 
     * @param rpn_bboxs 
     * @param anchors 
     */
    Matrix<float> getProposals(Matrix<float> rpn_probs, Matrix<float> rpn_bboxs, Matrix<float> anchors);

    Tensor<float> roiAlign(Matrix<float> rpn_rois, std::vector<Tensor<float>> mrcnn_feature_maps, int pool_size);

    Matrix<float> getRefineProposals(Matrix<float> rpn_rois, Matrix<float> refined_bboxs, Matrix<float> refined_probs);

    void unmoldMRCNN(Matrix<float> mrcnn_rois, 
                     Tensor<float> mrcnn_masks, 
                     int original_height, 
                     int original_width, 
                     int model_height, 
                     int model_width, 
                     Matrix<int> window, 
                     Matrix<int>& final_rois, 
                     Tensor<unsigned char>& final_masks);

    ModelRun* m_rpn_model;
    ModelRun* m_det_model;
    ModelRun* m_mask_model;

    int m_rpn_model_h;
    int m_rpn_model_w;
    std::string m_rpn_input_name;
    std::string m_det_input_name;
    std::string m_mask_input_name;

    int m_pre_num_limit;
    Matrix<float> m_anchors;

    float m_nms_rpn_thres;
    float m_nms_refine_thres;
    float m_seg_thres;
};
    
}
#endif