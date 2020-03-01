#ifndef _EAGLEEYE_IMAGETRANSFORMNODE_H_
#define _EAGLEEYE_IMAGETRANSFORMNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/processnode/ImageProcessNode.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"

namespace eagleeye{
class ImageTransformNode:public ImageProcessNode<ImageSignal<Array<unsigned char, 3>>, ImageSignal<Array<unsigned char, 3>>>{
public:
    typedef ImageTransformNode                                  Self;
    typedef ImageProcessNode<ImageSignal<Array<unsigned char, 3>>, ImageSignal<Array<unsigned char, 3>>>      Superclass;

    typedef ImageSignal<Array<unsigned char, 3>>    IMAGE_SIGNAL_TYPE;
    
    EAGLEEYE_CLASSIDENTITY(ImageTransformNode);

    EAGLEEYE_INPUT_PORT_TYPE(IMAGE_SIGNAL_TYPE,         0,      INPUT_IMAGE);
    EAGLEEYE_OUTPUT_PORT_TYPE(IMAGE_SIGNAL_TYPE,        0,      OUTPUT_IMAGE);

    /**
	 *	@brief execute ImageTransformNode algorithm
     *  @note user must finish this function
	 */
    ImageTransformNode(bool is_crop=false);
    virtual ~ImageTransformNode();

    /**
	 *	@brief execute Saliency algorithm
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    void setScale(float scale);
    void getScale(float& scale);

    void setMinSize(int size);
    void getMinSize(int& size);

private:
    /**
     * @brief Construct a new ImageTransformNode Node object
     * @note prohibit
     */
    ImageTransformNode(const ImageTransformNode&);
	void operator=(const ImageTransformNode&);

    float m_scale;
    int m_min_size;
    bool m_is_crop;
};
}
#endif