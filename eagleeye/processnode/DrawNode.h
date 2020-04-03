#ifndef _EAGLEEYE_DRAWNODE_H_
#define _EAGLEEYE_DRAWNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"


namespace eagleeye{
class DrawNode:public AnyNode{
public:
    typedef DrawNode                Self;
    typedef AnyNode                 Superclass;

    DrawNode();
    virtual ~DrawNode();

    /**
	 *	@brief execute 
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

private:
    /**
     * @brief Construct
     * @note prohibit
     */
    DrawNode(const DrawNode&);
	void operator=(const DrawNode&);
};
}
#endif