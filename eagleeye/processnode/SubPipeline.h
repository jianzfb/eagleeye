#ifndef _EAGLEEYE_SUBPIPELINE_H_
#define _EAGLEEYE_SUBPIPELINE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include <string>
#include <vector>
#include <map>

namespace eagleeye{
class SubPipeline:public AnyNode{
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
     * @param name 
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
     * @brief reset asynnode
     * 
     */
    virtual void reset();

    /**
	 *	@brief get monitor pool of the whole pipeline
	 *	@note traverse the whole pipeline
	 */
	virtual void getPipelineMonitors(std::map<std::string,std::vector<AnyMonitor*>>& pipeline_monitor_pool);

protected:
    std::map<std::string, AnyNode*> m_subpipeline;
    std::map<std::string, int> m_out_deg;
    std::map<std::string, int> m_in_deg;
    
    std::string m_output_node_name;
    AnyNode* m_output_node;


    std::vector<AnyNode*> m_input_nodes;
    std::vector<int> m_input_ports;

    AnySignal* m_input_node_placeholder;

private:
    SubPipeline(const SubPipeline&);
    void operator=(const SubPipeline&);
};
}
#endif