#ifndef _EAGLEEYE_CONSTNODE_H_
#define _EAGLEEYE_CONSTNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"


namespace eagleeye
{
class ConstNode:public AnyNode, DynamicNodeCreator<ConstNode>{
public:
    typedef ConstNode               Self;
    typedef AnyNode             Superclass;

    /**
	 *	@brief Get class identity
	 */
    EAGLEEYE_CLASSIDENTITY(ConstNode);

    /**
     *  @brief constructor/destructor
     */
    ConstNode(int input_port_num=0, std::vector<AnySignal*> output_const_sigs=std::vector<AnySignal*>());
    virtual ~ConstNode();

    virtual void executeNodeInfo();

private:
    ConstNode(const ConstNode&);
    void operator=(const ConstNode&);
};
} // namespace eagleeye

#endif