#ifndef _EAGLEEYE_ENHANCENODE_H_
#define _EAGLEEYE_ENHANCENODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"


namespace eagleeye
{
class EnhanceNode:public AnyNode, DynamicNodeCreator<EnhanceNode>{
public:
    typedef EnhanceNode             Self;
    typedef AnyNode                 Superclass;

    EAGLEEYE_CLASSIDENTITY(EnhanceNode);

    EnhanceNode();
    virtual ~EnhanceNode();

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

private:
    EnhanceNode(const EnhanceNode&);
    void operator=(const EnhanceNode&);

};    
} // namespace eagleeye


#endif