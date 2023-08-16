#ifndef _EAGLEEYE_STATE2BOOLEANNODE_H_
#define _EAGLEEYE_STATE2BOOLEANNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include <map>
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"


namespace eagleeye{
class State2BooleanNode:public AnyNode, DynamicNodeCreator<State2BooleanNode>{
public:
    typedef State2BooleanNode                       Self;
    typedef AnyNode                                 Superclass;

    State2BooleanNode(std::map<int,bool> state_2_bool=std::map<int,bool>());
    virtual ~State2BooleanNode();

    /**
	 *	@brief Get class identity
	*/
	EAGLEEYE_CLASSIDENTITY(State2BooleanNode);   

    /**
	 * @brief execute control
	 * 
	 */
	virtual void executeNodeInfo();

private:
    State2BooleanNode(const State2BooleanNode&);
    void operator=(const State2BooleanNode&);    

    std::map<int,bool> m_state_2_bool;
};

}
#endif