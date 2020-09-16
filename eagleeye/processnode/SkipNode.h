#ifndef _EAGLEEYE_SKIPNODE_H_
#define _EAGLEEYE_SKIPNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
namespace eagleeye{
class SkipNode:public AnyNode{
public:
	/**
	 *	@brief define some basic type
	 *	@note you must do these
	 */
	typedef SkipNode								Self;
	typedef AnyNode								    Superclass;

    SkipNode(std::function<AnyNode*()> generator);
    virtual ~SkipNode();

    /**
	 *	@brief Get class identity
	 */
	EAGLEEYE_CLASSIDENTITY(SkipNode);

    /**
     * @brief overide setUnitName
     */
	virtual void setUnitName(const char* unit_name);

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
     * @brief add/set input port
     * 
     * @param sig 
     */
    virtual void addInputPort(AnySignal* sig);
	virtual void setInputPort(AnySignal* sig,int index=0);
    
    /**
     * @brief Set/Get the Skip object
     * 
     * @param skip 
     */
    void setSkip(bool skip);
    void getSkip(bool& skip);

private:
    SkipNode(const SkipNode&);
    void operator=(const SkipNode&);

    bool m_skip;
    AnyNode* m_execute_node;
};
}
#endif