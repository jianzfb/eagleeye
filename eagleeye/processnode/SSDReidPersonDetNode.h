#ifndef _EAGLEEYE_SSDREIDDETNODE_H_
#define _EAGLEEYE_SSDREIDDETNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/processnode/ImageProcessNode.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/engine/proxy_run.h"
#include "eagleeye/basic/Tensor.h"


namespace eagleeye{
class SSDReIDPersonDetNode:public AnyNode{
public:
    typedef SSDReIDPersonDetNode                  Self;
    typedef AnyNode                             Superclass;
    
    typedef typename ImageSignal<Array<unsigned char, 3>>::MetaType		InputDataType;
	typedef typename ImageSignal<float>::MetaType					    OutputDataType;

    typedef ImageSignal<Array<unsigned char, 3>>    IMAGE_SIGNAL_TYPE;
    typedef ImageSignal<float>                      BOX_SIGNAL_TYPE;
    typedef ImageSignal<float>                      FEATURE_SIGNAL_TYPE;

    /**
     * @brief get class identity
     * 
     */
    EAGLEEYE_CLASSIDENTITY(SSDReIDPersonDetNode);

    /**
     * @brief define input/output port
     * 
     */
    EAGLEEYE_INPUT_PORT_TYPE(IMAGE_SIGNAL_TYPE,      0,      FRAME);
    EAGLEEYE_OUTPUT_PORT_TYPE(BOX_SIGNAL_TYPE,       0,      BOX);
    EAGLEEYE_OUTPUT_PORT_TYPE(FEATURE_SIGNAL_TYPE,   1,      REIDFEATURE);

    SSDReIDPersonDetNode(bool enable_reid=true);
    virtual ~SSDReIDPersonDetNode();

    /**
	 *	@brief execute goturn algorithm
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    /**
     * @brief Set/Get the Writable Path object
     * 
     * @param path 
     */
    void setModelPath(std::string path);
    void getModelPath(std::string& path);

    /**
     * @brief Set/Get NSM score thres
     * 
     * @param score 
     */
    void setNSMScoreThres(float score);
    void getNSMScoreThres(float& score);

    /**
     * @brief Set/Get IOU score thres
     * 
     * @param iou 
     */
    void setIOUScoreThres(float iou);
    void getIOUScoreThres(float& iou);

private:
    SSDReIDPersonDetNode(const SSDReIDPersonDetNode&);
    void operator=(const SSDReIDPersonDetNode&);

    Matrix<float> getAllAnchors();
    Matrix<float> _tile_anchors(unsigned int grid_height,
                                      unsigned int grid_width,
                                      Matrix<float> scales,
                                      Matrix<float> aspect_ratios,
                                      std::vector<float> base_anchor_size,
                                      std::vector<float> anchor_stride,
                                      std::vector<float> anchor_offset);

    Matrix<float> decodePosition(Matrix<float> prediction_loc, 
                                    Matrix<float> person_cls,
                                    int o_h,
                                    int o_w);
    ModelRun* m_det;
    ModelRun* m_reid;
    int m_h;
    int m_w;
    float m_nms_iou_thres;
    float m_nms_conf_thres;
    int m_max_num;
    int m_class_num;
    Matrix<float> m_anchors;

    std::vector<std::vector<int64_t>> m_output_shapes;
    std::vector<std::string> m_output_names;
    Matrix<Array<float,3>> m_model_image_f;
    bool m_enable_reid;
};

}
#endif