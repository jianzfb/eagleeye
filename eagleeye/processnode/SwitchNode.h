#ifndef _EAGLEEYE_SWITCHNODE_H_
#define _EAGLEEYE_SWITCHNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include <vector>
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"

namespace eagleeye
{
class SwitchNode:public AnyNode, DynamicNodeCreator<SwitchNode>{
public:
    typedef SwitchNode              Self;
    typedef AnyNode             Superclass;    

    EAGLEEYE_CLASSIDENTITY(SwitchNode);

    SwitchNode(std::vector<AnyNode*> candidates=std::vector<AnyNode*>());
    virtual ~SwitchNode();

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();
    
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
    SwitchNode(const SwitchNode&);
    void operator=(const SwitchNode&);

    std::vector<AnyNode*> m_candidates;
}; 
} // namespace eagleeye


#endif