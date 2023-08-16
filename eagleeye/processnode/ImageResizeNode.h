#ifndef _EAGLEEYE_IMAGERESIZENODE_H_
#define _EAGLEEYE_IMAGERESIZENODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"


namespace eagleeye
{
class ImageResizeNode: public AnyNode, DynamicNodeCreator<ImageResizeNode>{
    typedef ImageResizeNode                       Self;
    typedef AnyNode                             Superclass;

    EAGLEEYE_CLASSIDENTITY(ImageResizeNode);

public:
    ImageResizeNode(int resize_w=0, int resize_h=0, float resize_scale=0.0f);
    virtual ~ImageResizeNode();

    /**
	 *	@brief execute goturn algorithm
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    void setResizeW(int w);
    void getResizeW(int& w);

    void setResizeH(int h);
    void getResizeH(int& h);

private:
    ImageResizeNode(const ImageResizeNode&);
    void operator=(const ImageResizeNode&); 

    int m_resize_w;
    int m_resize_h;   
    float m_resize_scale;
};    
} // namespace eagleeye


#endif