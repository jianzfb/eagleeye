#ifndef _EAGLEEYE_IMAGECONTAINER_H_
#define _EAGLEEYE_IMAGECONTAINER_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/render/RenderNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include <GLES3/gl3.h>
#include "eagleeye/render/ImageShow.h"
#include <vector>

namespace eagleeye
{
class ImageContainer:public RenderNode{
public:
    typedef ImageContainer                   Self;
    typedef RenderNode                  Superclass;
    EAGLEEYE_CLASSIDENTITY(ImageContainer);

    ImageContainer();
    virtual ~ImageContainer();

    void setHSplit(int split_h);
    void getHSplit(int& split_h);

    void setWSplit(int split_w);
    void getWSplit(int& split_w);

    void setMarginX(int margin_x);
    void getMarginX(int& margin_x);

    void setMarginY(int margin_y);
    void getMarginY(int& margin_y);

    void setPaddingX(int padding_x);
    void getPaddingX(int& padding_x);

    void setPaddingY(int padding_y);
    void getPaddingY(int& padding_y);

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    /**
     * @brief init gl 
     */ 
    virtual void init();  

private:
    ImageContainer(const ImageContainer&);
    void operator=(const ImageContainer&);

    int m_split_w;
    int m_split_h;

    int m_margin_x;
    int m_margin_y;
    int m_padding_x;
    int m_padding_y;
    std::vector<std::shared_ptr<ImageShow>> m_imageshow_list;
}; 
} // namespace eagleeye

#endif