#ifndef _EAGLEEYE_SERIALIZE_NODE_H_
#define _EAGLEEYE_SERIALIZE_NODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include <vector>

namespace eagleeye{
class SerializeNode:public AnyNode{
public:
    typedef SerializeNode              Self;
    typedef AnyNode                         Superclass;    

    EAGLEEYE_CLASSIDENTITY(SerializeNode);

    SerializeNode();
    virtual ~SerializeNode();

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();


private:
    SerializeNode(const SerializeNode&);
    void operator=(const SerializeNode&);
}; 
}
#endif