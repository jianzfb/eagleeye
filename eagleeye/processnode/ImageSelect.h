#ifndef _EAGLEEYE_IMAGESELECT_H_
#define _EAGLEEYE_IMAGESELECT_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/processnode/ImageReadNode.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/Array.h"
#include <vector>
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"


namespace eagleeye{
class ImageSelect:public ImageReadNode, DynamicNodeCreator<ImageSelect>{
public:
    typedef ImageSelect           Self;
    typedef ImageReadNode                Superclass;

    /**
	 *	@brief Get class identity
	 */
    EAGLEEYE_CLASSIDENTITY(ImageSelect);    

    /**
     *  @brief constructor/destructor
     */
    ImageSelect(std::vector<std::string> image_list=std::vector<std::string>{});
    virtual ~ImageSelect();

    void setIndex(int index);
    void getIndex(int& index);

    /**
	 * @brief execute node 
	 * 
	 */
    virtual void executeNodeInfo();

    /**
     * @brief 初始化函数
     * 
     */
    virtual void init();

private:
    ImageSelect(const ImageSelect&);
    void operator=(const ImageSelect&);

    int m_background_index;
    std::vector<std::string> background_img_list;
};
}

#endif