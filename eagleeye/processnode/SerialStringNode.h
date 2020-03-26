#ifndef _EAGLEEYE_SERIALSTRINGNODE_H_
#define _EAGLEEYE_SERIALSTRINGNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/common/EagleeyeSerial.h"
#include <string>

namespace eagleeye{
class SerialStringNode: public AnyNode{
public:
    typedef SerialStringNode                    Self;
    typedef AnyNode                             Superclass;

    /**
	 *	@brief get class identity	
	 */	 
	EAGLEEYE_CLASSIDENTITY(SerialStringNode);

    SerialStringNode();
    virtual ~SerialStringNode();

    /**
     * @brief read line from file
     * 
     */
	virtual void executeNodeInfo();

    /**
     * @brief Set/Get the Folder Path object
     * 
     * @param folder_path 
     */
    void setFolder(std::string folder_path);
    void getFolder(std::string& folder_path);

    /**
     * @brief Set/Get the Prefix object
     * 
     * @param prefix 
     */
    void setPrefix(std::string prefix);
    void getPrefix(std::string& prefix);

    /**
     * @brief feadback
     * 
     * @param node_state_map 
     */
    void feadback(std::map<std::string, int>& node_state_map);

private:
    SerialStringNode(const SerialStringNode&);
    void operator=(const SerialStringNode&);

    std::string m_folder;
    std::string m_prefix;

    SerialStringReader* m_serial_string_reader;
    bool m_is_change;
};
}
#endif