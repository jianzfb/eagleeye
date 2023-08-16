#ifndef _EAGLEEYE_READLINEFROMFILENODE_H_
#define _EAGLEEYE_READLINEFROMFILENODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include <string>
#include <vector>
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"

namespace eagleeye{
class ReadLineFromFileNode:public AnyNode, DynamicNodeCreator<ReadLineFromFileNode>{
public:
    typedef ReadLineFromFileNode                Self;
    typedef AnyNode                             Superclass;

    /**
	 *	@brief get class identity	
	 */	 
	EAGLEEYE_CLASSIDENTITY(ReadLineFromFileNode);

    ReadLineFromFileNode();
    virtual ~ReadLineFromFileNode();

    /**
     * @brief read line from file
     * 
     */
	virtual void executeNodeInfo();

    /**
     * @brief Set/Get the File Path object
     * 
     * @param file_path 
     */
    virtual void setFilePath(std::string file_path);
    virtual void getFilePath(std::string& file_path);
    
    // /**
    //  * @brief feadback
    //  * 
    //  * @param node_state_map 
    //  */
    // void feadback(std::map<std::string, int>& node_state_map);

    virtual bool finish();

private:
    ReadLineFromFileNode(const ReadLineFromFileNode&);
    void operator=(const ReadLineFromFileNode&);
    std::string m_file_path;
    std::vector<std::string> m_file_list;
    int m_cur_index;
};
}
#endif