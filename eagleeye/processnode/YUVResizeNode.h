#ifndef _EAGLEEYE_YUVRESIZENODE_H_
#define _EAGLEEYE_YUVRESIZENODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"

namespace eagleeye
{
class YUVResizeNode: public AnyNode{
    typedef YUVResizeNode                       Self;
    typedef AnyNode                             Superclass;

    EAGLEEYE_CLASSIDENTITY(YUVResizeNode);

public:
    YUVResizeNode(int resize_w=360, int resize_h=640);
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
};    
} // namespace eagleeye


#endif