#ifndef _EAGLEEYE_PROXYNODE_H_
#define _EAGLEEYE_PROXYNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/spinlock.hpp"
#include <vector>
#include <map>
#include <memory>
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"

namespace eagleeye{
class ProxyNode:public AnyNode, DynamicNodeCreator<ProxyNode>{
public:
    typedef ProxyNode                   Self;
    typedef AnyNode                     Superclass;
    EAGLEEYE_CLASSIDENTITY(ProxyNode);

    ProxyNode(std::function<AnyNode*()> generator=nullptr);
    virtual ~ProxyNode();

    /**
     * @brief overide setUnitName
     */
	virtual void setUnitName(const char* unit_name);

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

	virtual void setNumberOfInputSignals(unsigned int inputnum);

private:
    ProxyNode(const ProxyNode&);
    void operator=(const ProxyNode&);

    std::vector<AnySignal*> m_cache_signals;
    AnyNode* m_func_node;
}; 
}

#endif