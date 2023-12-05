#ifndef _EAGLEEYE_CALLBACK_NODE_H_
#define _EAGLEEYE_CALLBACK_NODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include <vector>

namespace eagleeye{
class CallbackNode:public AnyNode{
public:
    typedef CallbackNode              Self;
    typedef AnyNode                 Superclass;    

    EAGLEEYE_CLASSIDENTITY(CallbackNode);

    CallbackNode(std::function<void(AnyNode*, std::vector<AnySignal*>)> callback=nullptr);
    virtual ~CallbackNode();

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    /**
     * @brief set call back
     */
	virtual void setCallback(std::function<void(AnyNode*, std::vector<AnySignal*>)> callback);

private:
    CallbackNode(const CallbackNode&);
    void operator=(const CallbackNode&);

    std::function<void(AnyNode*, std::vector<AnySignal*>)> m_callback;
}; 
}
#endif