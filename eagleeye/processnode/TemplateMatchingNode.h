#ifndef _EAGLEEYE_TEMPLATEMATCHING_H_
#define _EAGLEEYE_TEMPLATEMATCHING_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/common/EagleeyeOpenCL.h"

namespace eagleeye{
enum TMMode{
    TM_CCORR_NORMED = 0,
    TM_SQDIFF_NORMED
};

class TemplateMatchingNode:public AnyNode{
public:
    typedef TemplateMatchingNode            Self;
    typedef AnyNode                         Superclass;

    typedef ImageSignal<Array<unsigned char, 3>>    IMAGE_SIGNAL_TYPE;
    typedef ImageSignal<int>                        LOCATION_SIGNAL_TYPE;
    typedef ImageSignal<float>                      SCORE_SIGNAL_TYPE;

    EAGLEEYE_CLASSIDENTITY(TemplateMatchingNode);

    /**
     * @brief define input/output port
     * 
     */
    EAGLEEYE_INPUT_PORT_TYPE(IMAGE_SIGNAL_TYPE,         0,      FRAME);
    EAGLEEYE_OUTPUT_PORT_TYPE(SCORE_SIGNAL_TYPE,        0,      SCORE);

    TemplateMatchingNode();
    virtual ~TemplateMatchingNode();

    /**
     * @brief Set the Matching Template
     * 
     * @param mat 
     */
    void setTemplate(Matrix<Array<unsigned char,3>> mat);

    /**
	 *	@brief execute goturn algorithm
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    /**
     * @brief set/get tm mode
     * 
     * @param mode 
     */
    void setTMMode(int mode);
    void getTMMode(int& mode);

private:
    TemplateMatchingNode(const TemplateMatchingNode&);
    void operator=(const TemplateMatchingNode&);

#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
    EAGLEEYE_OPENCL_DECLARE_KERNEL_GROUP(TM);
#endif
    Matrix<Array<unsigned char, 3>> m_tmpl;
    bool m_tmpl_update;
    int m_mode;
    Matrix<Array<unsigned char, 4>> m_rgba_img;
    Matrix<float> m_matching_score;
};
}
#endif