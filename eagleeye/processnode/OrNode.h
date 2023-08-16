#ifndef _EAGLEEYE_ORNODE_H_
#define _EAGLEEYE_ORNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"

namespace eagleeye
{
class OrNode: public AnyNode, DynamicNodeCreator<OrNode>{
public:
    typedef OrNode              Self;
    typedef AnyNode             Superclass;    

    EAGLEEYE_CLASSIDENTITY(OrNode);

    OrNode();
    virtual ~OrNode();

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    void addInputPort(AnySignal* sig);
    void setInputPort(AnySignal* sig,int index);

protected:
    virtual bool isNeedProcessed();

private:
    OrNode(const OrNode&);
    void operator=(const OrNode&);
};    
} // namespace eagleeye

#endif