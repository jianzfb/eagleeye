#ifndef _EAGLEEYE_MESSAGE_NODE_H_
#define _EAGLEEYE_MESSAGE_NODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include <vector>

namespace eagleeye{
class MessageNode:public AnyNode{
public:
    typedef MessageNode                     Self;
    typedef AnyNode                         Superclass;    

    EAGLEEYE_CLASSIDENTITY(MessageNode);

    MessageNode();
    virtual ~MessageNode();

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();


private:
    MessageNode(const MessageNode&);
    void operator=(const MessageNode&);
}; 
}
#endif