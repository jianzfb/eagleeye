#ifndef _EAGLEEYE_IMAGEWRITENODE_H_
#define _EAGLEEYE_IMAGEWRITENODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Matrix.h"
#include <string>

namespace eagleeye{
class ImageWriteNode:public AnyNode{
public:
    typedef ImageWriteNode      Self;
    typedef AnyNode             Superclass;

    ImageWriteNode();
    virtual ~ImageWriteNode();

    EAGLEEYE_CLASSIDENTITY(ImageWriteNode);

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
    void setWriteFolder(std::string folder);
    void getWriteFolder(std::string& folder);

    /**
	 * @brief reset pipeline
	 * 
	 */
	virtual void reset();

protected:
    void writePngFile(const char* file_path, unsigned char* data, int height, int width, int stride, int channel);

    /**
     * @brief Get the File Name object
     * 
     * @return std::string 
     */
    std::string getFileNamePrefix(std::string filename);

private:
    /**
     * @brief Construct a new ImageWriteNode Node object
     * @note prohibit
     */
    ImageWriteNode(const ImageWriteNode&);
	void operator=(const ImageWriteNode&);

    int m_count;
    std::string m_folder;
};
}
#endif