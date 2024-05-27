#ifndef _EAGLEEYE_FORNODE_H_
#define _EAGLEEYE_FORNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"
#include "eagleeye/framework/pipeline/GroupSignal.h"
#include <vector>
#include <map>
#include <memory>


namespace eagleeye{
class ForNode:public AnyNode, DynamicNodeCreator<ForNode>{
public:
    typedef ForNode                Self;
    typedef AnyNode                 Superclass;
    EAGLEEYE_CLASSIDENTITY(ForNode);

    ForNode(std::function<AnyNode*()> generator=nullptr);
    virtual ~ForNode();

    /**
     * @brief overide setUnitName
     */
	virtual void setUnitName(const char* unit_name);

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    void setFolder(const std::string folder);
    void getFolder(std::string& folder);

    /**
	 *	@brief get monitor pool of the whole pipeline
	 *	@note traverse the whole pipeline
	 */
	virtual void getPipelineMonitors(std::map<std::string,std::vector<AnyMonitor*>>& pipeline_monitor_pool);

private:
    ForNode(const ForNode&);
    void operator=(const ForNode&);

    AnyNode* m_auto_node;
};
}
#endif