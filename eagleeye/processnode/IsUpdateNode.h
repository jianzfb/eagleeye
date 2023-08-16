#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"


namespace eagleeye
{
class IsUpdateNode: public AnyNode, DynamicNodeCreator<IsUpdateNode>{
public:
    typedef IsUpdateNode                Self;
    typedef AnyNode                     Superclass;    

    EAGLEEYE_CLASSIDENTITY(IsUpdateNode);

    IsUpdateNode();
    virtual ~IsUpdateNode();

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();
    
private:
    IsUpdateNode(const IsUpdateNode&);
    void operator=(const IsUpdateNode&);
};    
} // namespace eagleeye
