#ifndef _EAGLEEYE_YUVRESIZENODE_H_
#define _EAGLEEYE_YUVRESIZENODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"

namespace eagleeye
{
class YUVResizeNode: public AnyNode, DynamicNodeCreator<YUVResizeNode>{
    typedef YUVResizeNode                       Self;
    typedef AnyNode                             Superclass;

    EAGLEEYE_CLASSIDENTITY(YUVResizeNode);

public:
    YUVResizeNode(int resize_w=0, int resize_h=0, float resize_scale=0.0f);
    virtual ~YUVResizeNode();

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
    YUVResizeNode(const YUVResizeNode&);
    void operator=(const YUVResizeNode&); 

    int m_resize_w;
    int m_resize_h;   
    float m_resize_scale;
};    
} // namespace eagleeye


#endif