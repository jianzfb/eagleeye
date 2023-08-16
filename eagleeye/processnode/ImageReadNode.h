#ifndef _EAGLEEYE_IMAGEREADNODE_H_
#define _EAGLEEYE_IMAGEREADNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include <string>
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"


namespace eagleeye{
class ImageReadNode:public AnyNode, DynamicNodeCreator<ImageReadNode>{
public:
    typedef ImageReadNode           Self;
    typedef AnyNode                 Superclass;

    ImageReadNode();
    virtual ~ImageReadNode();

    EAGLEEYE_CLASSIDENTITY(ImageReadNode);

    /**
	 *	@brief execute process
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    /**
     * @brief Set/Get the Write Folder object
     * 
     * @param folder 
     */
    void setImagePath(std::string image_path);
    void getImagePath(std::string& image_path);

protected:
    void readPngFile(const char *filename);

private:
    /**
     * @brief Construct a new ImageReadNode Node object
     * @note prohibit
     */
    ImageReadNode(const ImageReadNode&);
	void operator=(const ImageReadNode&);

    std::string m_image_path;
};
}
#endif