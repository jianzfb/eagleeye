#ifndef _EAGLEEYE_LOGICALNODE_H_
#define _EAGLEEYE_LOGICALNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"

namespace eagleeye{
enum LogicalType{
    LOGICAL_AND = 0,
    LOGICAL_OR = 1
};

class LogicalNode:public AnyNode{
public:
    typedef LogicalNode                 Self;
    typedef AnyNode                     Superclass;

    LogicalNode(LogicalType logical_type);
    virtual ~LogicalNode();

    /**
	 *	@brief Get class identity
	 */
	EAGLEEYE_CLASSIDENTITY(LogicalNode);

    /**
	 * @brief execute control
	 * 
	 */
	virtual void executeNodeInfo();

private:
    LogicalNode(const LogicalNode&);
    void operator=(const LogicalNode&);

    LogicalType m_logical_type;
}; 
}
#endif