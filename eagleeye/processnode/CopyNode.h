#ifndef _EAGLEEYE_COPYNODE_H_
#define _EAGLEEYE_COPYNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"

namespace eagleeye{
class CopyNode:public AnyNode{
public:
    typedef CopyNode                Self;
    typedef AnyNode                 Superclass;

    EAGLEEYE_CLASSIDENTITY(CopyNode);

    CopyNode(bool inplace=false);
    virtual ~CopyNode();

    /**
     * @brief add/set input port
     * 
     * @param sig 
     */
	virtual void addInputPort(AnySignal* sig);
	virtual void setInputPort(AnySignal* sig,int index=0);

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

private:
    CopyNode(const CopyNode&);
    void operator=(const CopyNode&);

    bool m_inplace;
};
}
#endif