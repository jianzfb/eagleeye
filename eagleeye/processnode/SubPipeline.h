#ifndef _EAGLEEYE_SUBPIPELINE_H_
#define _EAGLEEYE_SUBPIPELINE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/AnyPipeline.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"
#include <string>
#include <vector>
#include <map>

namespace eagleeye{
class SubPipeline:public AnyNode, DynamicNodeCreator<SubPipeline>{
public:
    typedef SubPipeline                     Self;
    typedef AnyNode                         Superclass;

    /**
     * @brief get class identity
     * 
     */
    EAGLEEYE_CLASSIDENTITY(SubPipeline);

    SubPipeline();
    virtual ~SubPipeline();

    /**
	 *	@brief execute goturn algorithm
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    /**
     * @brief add node in subpipeline
     * 
     * @param node 
     * @param nodetype 
     */
    void add(AnyNode* node, std::string name);

    /**
     * @brief connect two nodes
     * special node, "SOURCE", "SINK".
     * @param fromname 
     * @param fromport 
     * @param toname 
     * @param toport 
     */
    void bind(std::string fromname, int fromport, std::string toname, int toport);

    /**
     *  @brief analyze pipeline
     */
    void analyze();

    /**
     * @brief reset subpipeline
     * 
     */
    virtual void reset();

    /**
     * @brief init subpipeline
     * 
     */
    virtual void init();

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

protected:
    std::map<std::string, AnyNode*> m_subpipeline;
    std::vector<std::string> m_name_list;

    std::vector<std::string> m_input_node_name_list;
    std::vector<std::string> m_output_node_name_list;
    std::vector<AnySignal*> m_placeholders;

    std::map<std::string, int> m_node_bind_input_num;
    std::map<std::string, int> m_node_bind_output_num;

private:
    SubPipeline(const SubPipeline&);
    void operator=(const SubPipeline&);
};
}
#endif