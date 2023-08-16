#ifndef _EAGLEEYE_IMAGEROTATENODE_H_
#define _EAGLEEYE_IMAGEROTATENODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"

namespace eagleeye
{
class ImageRotateNode:public AnyNode, DynamicNodeCreator<ImageRotateNode>{
public:
    ImageRotateNode(float default_rot_deg=0.0f);
    virtual ~ImageRotateNode();

    /**
	 *	@brief execute goturn algorithm
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

private:
    ImageResizeNode(const ImageResizeNode&);
    void operator=(const ImageResizeNode&);     

    float m_default_rot_deg;
}; 
} // namespace eagleeye


#endif