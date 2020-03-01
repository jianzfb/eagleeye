#ifndef _EAGLEEYE_SIMRPNTRACKINGNODE_H_
#define _EAGLEEYE_SIMRPNTRACKINGNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/processnode/ImageProcessNode.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/engine/proxy_run.h"
#include "eagleeye/basic/Tensor.h"

namespace eagleeye{
class SimRPNTrackingNode:public AnyNode{
public:
    typedef SimRPNTrackingNode                  Self;
    typedef AnyNode                             Superclass;
    
    typedef typename ImageSignal<Array<unsigned char, 3>>::MetaType		InputDataType;
	typedef typename ImageSignal<float>::MetaType					    OutputDataType;

    typedef ImageSignal<Array<unsigned char, 3>>    IMAGE_SIGNAL_TYPE;
    typedef ImageSignal<float>                      BOX_SIGNAL_TYPE;

    /**
     * @brief get class identity
     * 
     */
    EAGLEEYE_CLASSIDENTITY(SimRPNTrackingNode);

    /**
     * @brief define input/output port
     * 
     */
    EAGLEEYE_INPUT_PORT_TYPE(IMAGE_SIGNAL_TYPE,      0,      FRAME);
    EAGLEEYE_INPUT_PORT_TYPE(BOX_SIGNAL_TYPE,        1,      BOX);
    EAGLEEYE_OUTPUT_PORT_TYPE(BOX_SIGNAL_TYPE,       0,      BOX);

    /**
     * @brief Construct a new Tracking Node object
     * 
     */
    SimRPNTrackingNode();
    virtual ~SimRPNTrackingNode();

    /**
	 *	@brief execute segmentation algorithm
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

	/**
	 *	@brief make self check
	 *	@note judge whether some preliminary conditions have been satisfied.
	 */
	virtual bool selfcheck();

    /**
     * @brief Set/Get the Writable Path object
     * 
     * @param path 
     */
    void setWritablePath(std::string path);
    void getWritablePath(std::string& path);

protected:
    Matrix<Array<unsigned char, 3>> getTemplateImage(Matrix<Array<unsigned char, 3>> frame, int* roi);
    Matrix<Array<unsigned char, 3>> getDetectImage(Matrix<Array<unsigned char, 3>> frame, int* roi, float& scale);
    Matrix<Array<unsigned char, 3>> getSubWindow(Matrix<Array<unsigned char, 3>> frame, int* roi,int cx,int cy, int s_z, bool is_template);

    Matrix<float> generateAnchor();
    Matrix<float> convertBbox(Matrix<float> delta, Matrix<float> anchor);
    Matrix<float> smooth(Matrix<float> predict_box, Matrix<float> predict_cls);

private:
    SimRPNTrackingNode(const SimRPNTrackingNode&);
    void operator=(const SimRPNTrackingNode&);

    ModelRun* m_model;
    ModelRun* m_tracking_model;
    Tensor<float> m_target_tck;
    Tensor<float> m_target_trk;

    Matrix<float> m_last_box;
    Matrix<float> m_last_center;

    Matrix<float> m_init_box;

    int m_instance_size;
    int m_exemplar_size;
    int m_anchor_stride;
    int m_track_base_size;
    int m_anchor_num;

    Matrix<float> m_anchors;
    float m_adaptive_lr;
    float TRACK_PENALTY_K;
    float TRACK_WINDOW_INFLUENCE;
    Matrix<float> m_window;
};


}


#endif