#ifndef _EAGLEEYE_IMAGETRANSFORMNODE_H_
#define _EAGLEEYE_IMAGETRANSFORMNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/processnode/ImageProcessNode.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"


namespace eagleeye{
class ImageTransformNode:public ImageProcessNode<ImageSignal<Array<unsigned char, 3>>, ImageSignal<Array<unsigned char, 3>>>, DynamicNodeCreator<ImageTransformNode>{
public:
    typedef ImageTransformNode                                  Self;
    typedef ImageProcessNode<ImageSignal<Array<unsigned char, 3>>, ImageSignal<Array<unsigned char, 3>>>      Superclass;

    EAGLEEYE_CLASSIDENTITY(ImageTransformNode);

    /**
	 *	@brief execute ImageTransformNode algorithm
     *  @note user must finish this function
	 */
    ImageTransformNode(std::vector<float> region=std::vector<float>(), std::vector<int> size=std::vector<int>());
    virtual ~ImageTransformNode();

    /**
	 *	@brief execute Saliency algorithm
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    /**
     * @brief add/set input port
     * 
     * @param sig 
     */
	virtual void addInputPort(AnySignal* sig);
	virtual void setInputPort(AnySignal* sig,int index=0);

    void setMinSize(int size);
    void getMinSize(int& size);

private:
    /**
     * @brief Construct a new ImageTransformNode Node object
     * @note prohibit
     */
    ImageTransformNode(const ImageTransformNode&);
	void operator=(const ImageTransformNode&);

    int m_min_size;
    std::vector<int> m_default_size;
    std::vector<float> m_default_region;
};
}
#endif