#ifndef _EAGLEEYE_YUVCONVERTNODE_H_
#define _EAGLEEYE_YUVCONVERTNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"


namespace eagleeye
{
enum YUVConvertType{
    YUV2RGB = 0,
    YUV2BGR = 1
};


class YUVConvertNode: public AnyNode{
public:
    typedef YUVConvertNode                     Self;
    typedef AnyNode                         Superclass;

    EAGLEEYE_CLASSIDENTITY(YUVConvertNode);

    YUVConvertNode(YUVConvertType convert_type=YUV2RGB);
    virtual ~YUVConvertNode();

    /**
	 *	@brief execute goturn algorithm
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();


private:
    YUVConvertNode(const YUVConvertNode&);
    void operator=(const YUVConvertNode&);

    YUVConvertType m_convert_type;
};
} // namespace eagleeye


#endif