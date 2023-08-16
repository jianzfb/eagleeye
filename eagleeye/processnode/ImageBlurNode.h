#ifndef _EAGLEEYE_IMAGEBLURNODE_H_
#define _EAGLEEYE_IMAGEBLURNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"

namespace eagleeye
{
class ImageBlurNode: public AnyNode, DynamicNodeCreator<ImageBlurNode>{
public:
    typedef ImageBlurNode               Self;
    typedef AnyNode                     Superclass;    

    EAGLEEYE_CLASSIDENTITY(ImageBlurNode);

    ImageBlurNode();
    virtual ~ImageBlurNode();

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

private:
    ImageBlurNode(const ImageBlurNode&);
    void operator=(const ImageBlurNode&);
};      
} // namespace eagleeye


#endif