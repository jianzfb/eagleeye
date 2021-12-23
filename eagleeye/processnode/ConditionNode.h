#ifndef _EAGLEEYE_CONDITIONNODE_H_
#define _EAGLEEYE_CONDITIONNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/BooleanSignal.h"
namespace eagleeye{
/**
 * @brief condition control node
 * 
 * @tparam SrcT 
 */
template<class SrcT>    
class ConditionNode:public AnyNode{
public:
    /**
	 *	@brief define some basic type
	 *	@note you must do these
	 */
	typedef ConditionNode								Self;
	typedef AnyNode								        Superclass;

    ConditionNode();
    virtual ~ConditionNode();

    /**
	 *	@brief Get class identity
	 */
	EAGLEEYE_CLASSIDENTITY(ConditionNode);

    /**
	 * @brief execute control
	 * 
	 */
	virtual void executeNodeInfo();

    /**
	 *	@brief make self check
	 *	@note judge whether some preliminary conditions have been satisfied.
	 */
	virtual bool selfcheck();

private:
    ConditionNode(const ConditionNode&);
    void operator=(const ConditionNode&);
};
}

#include "eagleeye/processnode/ConditionNode.hpp"
#endif