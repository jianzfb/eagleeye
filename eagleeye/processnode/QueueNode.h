#ifndef _EAGLEEYE_QUEUENODE_H_
#define _EAGLEEYE_QUEUENODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include <vector>
#include <queue>

namespace eagleeye{
class QueueNode:public AnyNode{
public:
    typedef QueueNode               Self;
    typedef AnyNode                 Superclass;

    EAGLEEYE_CLASSIDENTITY(QueueNode);
    
    QueueNode();
    virtual ~QueueNode();

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    /**
     * @brief add/set input port
     * 
     * @param sig 
     */
	virtual void addInputPort(AnySignal* sig);
	virtual void setInputPort(AnySignal* sig,int index=0);

private:
    QueueNode(const QueueNode&);
    void operator=(const QueueNode&);
};

}
#endif