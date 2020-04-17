#ifndef _EAGLEEYE_COPYNODE_H_
#define _EAGLEEYE_COPYNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"

namespace eagleeye{
template<class T>
class CopyNode:public AnyNode{
public:
    typedef CopyNode                Self;
    typedef AnyNode                 Superclass;

    EAGLEEYE_CLASSIDENTITY(CopyNode);

    CopyNode(){
        this->setNumberOfOutputSignals(1);
        this->setOutputPort(new T, 0);

        this->setNumberOfInputSignals(1);
    }
    virtual ~CopyNode(){

    }

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo(){
        this->getOutputPort(0)->copy(this->getInputPort(0));
    }

private:
    CopyNode(const CopyNode&);
    void operator=(const CopyNode&);
};
}
#endif