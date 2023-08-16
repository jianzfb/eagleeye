#ifndef _EAGLEEYE_GROUPNODE_H_
#define _EAGLEEYE_GROUPNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"


namespace eagleeye
{
class GroupNode:public AnyNode, DynamicNodeCreator<GroupNode>{
public:
    typedef GroupNode               Self;
    typedef AnyNode             Superclass;

    /**
	 *	@brief Get class identity
	 */
    EAGLEEYE_CLASSIDENTITY(GroupNode);

    /**
     *  @brief constructor/destructor
     */
    GroupNode();
    virtual ~GroupNode();

    virtual void executeNodeInfo();

    /**
     * @brief add/set input port
     * 
     * @param sig 
     */
	virtual void addInputPort(AnySignal* sig);
	virtual void setInputPort(AnySignal* sig,int index=0);


private:
    GroupNode(const GroupNode&);
    void operator=(const GroupNode&);
};
} // namespace eagleeye
#endif