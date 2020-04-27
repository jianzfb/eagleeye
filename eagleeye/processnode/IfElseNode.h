#ifndef _EAGLEEYE_IFELSENODE_H_
#define _EAGLEEYE_IFELSENODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/BooleanSignal.h"

namespace eagleeye{
/**
 * @brief if ... else ... control node
 */ 

class IfElseNode:public AnyNode{
public:
    /**
	 *	@brief define some basic type
	 *	@note you must do these
	 */
	typedef IfElseNode								    Self;
	typedef AnyNode								        Superclass;

    IfElseNode(AnyNode* x, AnyNode* y);
    virtual ~IfElseNode();

    /**
	 *	@brief Get class identity
	 */
	EAGLEEYE_CLASSIDENTITY(IfElseNode);

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

    /**
     * @brief init node
     * 
     */
    virtual void init();

	/**
     * @brief reset
     * 
     */
    virtual void reset();

    /**
     * @brief exit subpipeline
     * 
     */
    virtual void exit();
	
    /**
	 *	@brief get monitor pool of the whole pipeline
	 *	@note traverse the whole pipeline
	 */
	virtual void getPipelineMonitors(std::map<std::string,std::vector<AnyMonitor*>>& pipeline_monitor_pool);

private:
    IfElseNode(const IfElseNode&);
    void operator=(const IfElseNode&);

    AnyNode* m_x;
    AnyNode* m_y;

	std::vector<AnySignal*> m_placeholders;
};    
}
#endif