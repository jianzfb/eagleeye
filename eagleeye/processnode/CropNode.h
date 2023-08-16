#ifndef _EAGLEEYE_CROPNODE_H_
#define _EAGLEEYE_CROPNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"

namespace eagleeye{
class CropNode:public AnyNode, DynamicNodeCreator<CropNode>{
public:
    typedef CropNode                Self;
    typedef AnyNode                 Superclass;

    EAGLEEYE_CLASSIDENTITY(CropNode);

    CropNode(std::vector<float> region=std::vector<float>());
    virtual ~CropNode();

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
    CropNode(const CropNode&);
    void operator=(const CropNode&);

    std::vector<float> m_default_region;
};
}
#endif