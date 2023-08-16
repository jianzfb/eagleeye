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
enum SubPipelineNode{
    SOURCE_NODE = 0,
    SINK_NODE   = 1,
    OTHER_NODE  = 2
};

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
    void add(AnyNode* node, std::string name, SubPipelineNode nodetype=OTHER_NODE);

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

    /**
	 * @brief load/save pipeline configure
	 * 
	 * @param node_config 
	 */
	virtual void loadConfigure(std::map<std::string, std::shared_ptr<char>> nodes_config);
	virtual void saveConfigure(std::map<std::string, std::shared_ptr<char>>& nodes_config);

protected:
    std::map<std::string, AnyNode*> m_subpipeline;
    std::map<int, std::vector<std::pair<std::string, int>>> m_special_source_port_map;

    AnyNode* m_sink_node;
    int m_sink_ignore_port;
    int* m_sink_port_map;

    std::vector<AnySignal*> m_placeholders;

private:
    SubPipeline(const SubPipeline&);
    void operator=(const SubPipeline&);
};
}
#endif