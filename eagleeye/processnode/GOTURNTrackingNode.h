#ifndef _EAGLEEYE_GOTURNTRACKINGNODE_H_
#define _EAGLEEYE_GOTURNTRACKINGNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/processnode/ImageProcessNode.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/engine/proxy_run.h"
#include "eagleeye/basic/Tensor.h"

namespace eagleeye{
class GOTURNTrackingNode:public AnyNode{
public:
    typedef GOTURNTrackingNode                  Self;
    typedef AnyNode                             Superclass;
    
    typedef typename ImageSignal<Array<unsigned char, 3>>::MetaType		InputDataType;
	typedef typename ImageSignal<float>::MetaType					    OutputDataType;

    typedef ImageSignal<Array<unsigned char, 3>>    IMAGE_SIGNAL_TYPE;
    typedef ImageSignal<int>                        BOX_SIGNAL_TYPE;
    typedef ImageSignal<float>                      SCORE_SIGNAL_TYPE;

    /**
     * @brief get class identity
     * 
     */
    EAGLEEYE_CLASSIDENTITY(GOTURNTrackingNode);

    /**
     * @brief define input/output port
     * 
     */
    EAGLEEYE_INPUT_PORT_TYPE(IMAGE_SIGNAL_TYPE,      0,      FRAME);
    EAGLEEYE_INPUT_PORT_TYPE(BOX_SIGNAL_TYPE,        1,      BOX);
    EAGLEEYE_OUTPUT_PORT_TYPE(BOX_SIGNAL_TYPE,       0,      BOX);
    EAGLEEYE_OUTPUT_PORT_TYPE(SCORE_SIGNAL_TYPE,     1,      SCORE);


    /**
     * @brief Construct a new Tracking Node object
     * 
     */
    GOTURNTrackingNode(EagleeyeRuntimeType runtime);
    virtual ~GOTURNTrackingNode();

    /**
	 *	@brief execute goturn algorithm
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
    void setModelPath(std::string path);
    void getModelPath(std::string& path);

protected:
    Matrix<Array<unsigned char, 3>> getTemplateImage(Matrix<Array<unsigned char, 3>> frame, float* roi);
    Matrix<Array<unsigned char, 3>> getSearchImage(Matrix<Array<unsigned char, 3>> frame, float* roi);
    Matrix<Array<unsigned char, 3>> getSubWindow(Matrix<Array<unsigned char, 3>> frame, 
                                                 int* roi,
                                                 int init_w,
                                                 int init_h,
                                                 int& context_xmin, 
                                                 int& context_ymin, 
                                                 int& context_xmax,
                                                 int& context_ymax,
                                                 int& top_pad, 
                                                 int& left_pad);

private:
    GOTURNTrackingNode(const GOTURNTrackingNode&);
    void operator=(const GOTURNTrackingNode&);

    ModelRun* m_template_model;
    ModelRun* m_tracking_model;
    Matrix<int> m_last_box;
    Matrix<float> m_last_box_score;
    Tensor<float> m_template_tensor;
    int m_init_w;
    int m_init_h;

    Matrix<float> m_hanning_window;
    int m_template_size;
    bool is_init;
};
    
}


#endif