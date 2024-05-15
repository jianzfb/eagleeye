#ifndef _EAGLEEYE_STRINGREPLY_NODE_H_
#define _EAGLEEYE_STRINGREPLY_NODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include <vector>

namespace eagleeye{
class StringReplyNode:public AnyNode{
public:
    typedef StringReplyNode              Self;
    typedef AnyNode                         Superclass;    

    EAGLEEYE_CLASSIDENTITY(StringReplyNode);

    StringReplyNode();
    virtual ~StringReplyNode();

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    /**
     * @brief set call back
     */
	virtual void setCallback(std::function<void(std::string)> callback);

private:
    StringReplyNode(const StringReplyNode&);
    void operator=(const StringReplyNode&);

    std::function<void(std::string)> callback;
}; 
}
#endif