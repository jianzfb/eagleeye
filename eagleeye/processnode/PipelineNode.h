#ifndef _EAGLEEYE_PIPELINENODE_H_
#define _EAGLEEYE_PIPELINENODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/AnyPipeline.h"
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <thread>


namespace eagleeye{
class PipelineNode:public AnyNode{
public:
    typedef PipelineNode                Self;
    typedef AnyNode                     Superclass;
    EAGLEEYE_CLASSIDENTITY(PipelineNode);

    PipelineNode(std::function<AnyPipeline*()> generator, std::vector<std::pair<std::string, int>> export_node);
    virtual ~PipelineNode();

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    /**
     * @brief overide setUnitName
     */
	virtual void setUnitName(const char* unit_name);

private:
    PipelineNode(const PipelineNode&);
    void operator=(const PipelineNode&);

    AnyPipeline* m_auto_pipeline;
    std::vector<std::pair<std::string, int>> m_pipeline_node;
    std::vector<AnySignal*> m_cache_input;
}; 
}
#endif