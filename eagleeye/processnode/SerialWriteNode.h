#ifndef _EAGLEEYE_SerialWriteNode_H_
#define _EAGLEEYE_SerialWriteNode_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/common/EagleeyeSerial.h"
#include <string>
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"


namespace eagleeye{
class SerialWriteNode:public AnyNode, DynamicNodeCreator<SerialWriteNode>{
public:
    typedef SerialWriteNode              Self;
    typedef AnyNode                     Superclass;

    EAGLEEYE_CLASSIDENTITY(SerialWriteNode);

    /**
     * @brief Construct a new Serial Read Node object
     * 
     */
    SerialWriteNode();
    virtual ~SerialWriteNode();

    /**
	 *	@brief parse video data
	 */
	virtual void executeNodeInfo();

    /**
     * @brief Set/Get the folder
     * 
     * @param file_path 
     */
    virtual void setFolder(std::string folder);
    virtual void getFolder(std::string& folder);

    /**
     * @brief Set/Get the Prefix object
     * 
     * @param prefix 
     */
    virtual void setPrefix(std::string prefix);
    virtual void getPrefix(std::string& prefix);

private:
    SerialWriteNode(const SerialWriteNode&);
    void operator=(const SerialWriteNode&);
    
    std::string m_folder;
    std::string m_prefix;
    bool m_folder_or_prefix_update;
    int m_order_index;
};  
}
#endif