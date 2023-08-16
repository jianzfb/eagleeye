#ifndef _EAGLEEYE_IDENTITY_H_
#define _EAGLEEYE_IDENTITY_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"

namespace eagleeye
{
class IdentityNode:public AnyNode, DynamicNodeCreator<IdentityNode>{
public:
    typedef IdentityNode               Self;
    typedef AnyNode             Superclass;

    /**
	 *	@brief Get class identity
	 */
    EAGLEEYE_CLASSIDENTITY(IdentityNode);

    /**
     *  @brief constructor/destructor
     */
    IdentityNode();
    virtual ~IdentityNode();

    virtual void executeNodeInfo();

    /**
     * @brief add/set input port
     * 
     * @param sig 
     */
	virtual void addInputPort(AnySignal* sig);
	virtual void setInputPort(AnySignal* sig,int index=0);


private:
    IdentityNode(const IdentityNode&);
    void operator=(const IdentityNode&);    
};
} // namespace eagleeye

#endif