#ifndef _EAGLEEYE_YUVCONVERTNODE_H_
#define _EAGLEEYE_YUVCONVERTNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"


namespace eagleeye
{
enum YUVConvertType{
    I420ToRGB = 0,
    I420ToBGR = 1,
    NV21ToRGB = 2,
    NV21ToBGR = 3,
    NV12ToRGB = 4,
    NV12ToBGR = 5
};


class YUVConvertNode: public AnyNode, DynamicNodeCreator<YUVConvertNode>{
public:
    typedef YUVConvertNode                      Self;
    typedef AnyNode                             Superclass;

    EAGLEEYE_CLASSIDENTITY(YUVConvertNode);

    YUVConvertNode(YUVConvertType convert_type=I420ToRGB);
    virtual ~YUVConvertNode();

    /**
	 *	@brief execute goturn algorithm
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    void setConvertType(int convert_type);
    void getConvertType(int& convert_type);

private:
    YUVConvertNode(const YUVConvertNode&);
    void operator=(const YUVConvertNode&);

    YUVConvertType m_convert_type;
    int m_t;
};
} // namespace eagleeye


#endif