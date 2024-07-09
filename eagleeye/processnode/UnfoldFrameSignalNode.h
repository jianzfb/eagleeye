#ifndef _EAGLEEYE_UNFOLD_FRAMESIGNALNODE_H_
#define _EAGLEEYE_UNFOLD_FRAMESIGNALNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"

namespace eagleeye{
class UnfoldFrameSignalNode:public AnyNode, DynamicNodeCreator<UnfoldFrameSignalNode>{
public:
    typedef UnfoldFrameSignalNode       Self;
    typedef AnyNode                     Superclass;

    /**
	 *	@brief Get class identity
	 */
    EAGLEEYE_CLASSIDENTITY(UnfoldFrameSignalNode);

    /**
     *  @brief constructor/destructor
     */
    UnfoldFrameSignalNode();
    virtual ~UnfoldFrameSignalNode();

    virtual void executeNodeInfo();

    virtual void setInputPort(AnySignal* sig,int index=0);

private:
    UnfoldFrameSignalNode(const UnfoldFrameSignalNode&);
    void operator=(const UnfoldFrameSignalNode&);
};
}
#endif