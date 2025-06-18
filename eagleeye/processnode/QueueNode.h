#ifndef _EAGLEEYE_QUEUENODE_H_
#define _EAGLEEYE_QUEUENODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include <vector>
#include <queue>
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"

namespace eagleeye{
class QueueNode:public AnyNode, DynamicNodeCreator<QueueNode>{
public:
    typedef QueueNode               Self;
    typedef AnyNode                 Superclass;

    EAGLEEYE_CLASSIDENTITY(QueueNode);
    
    QueueNode(int queue_size=5, bool get_then_auto_remove=true, bool set_then_auto_remove=false);
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

    virtual void postexit();

private:
    QueueNode(const QueueNode&);
    void operator=(const QueueNode&);
    int m_queue_size;
    bool m_set_then_auto_remove;
    bool m_get_then_auto_remove;
};

}
#endif