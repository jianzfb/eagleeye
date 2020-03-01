#ifndef _EAGLEEYE_SERIALREADNODE_H_
#define _EAGLEEYE_SERIALREADNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/common/EagleeyeSerial.h"
#include <string>


namespace eagleeye{
class SerialReadNode:public AnyNode{
public:
    typedef SerialReadNode              Self;
    typedef AnyNode                     Superclass;

    EAGLEEYE_CLASSIDENTITY(SerialReadNode);

    /**
     * @brief Construct a new Serial Read Node object
     * 
     */
    SerialReadNode(EagleeyeType data_type);
    virtual ~SerialReadNode();

    /**
	 *	@brief parse video data
	 */
	virtual void executeNodeInfo();

    /**
     * @brief feadback 
     * 
     * @param node_state_map 
     */
	virtual void feadback(std::map<std::string, int>& node_state_map);

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

    /**
     * @brief self check
     * 
     * @return true 
     * @return false 
     */
    bool selfcheck();

private:
    SerialReadNode(const SerialReadNode&);
    void operator=(const SerialReadNode&);
    
    std::string m_folder;
    std::string m_prefix;
    bool m_folder_or_prefix_update;
    std::string m_next;
    SerialStringReader* m_serial_reader;
    EagleeyeType m_data_type;

    bool m_not_ok;
};  
}
#endif