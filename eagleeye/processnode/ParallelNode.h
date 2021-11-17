#ifndef _EAGLEEYE_PARALLELNODE_H_
#define _EAGLEEYE_PARALLELNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/processnode/SubPipeline.h"
#include <vector>
#include <thread>
#include <queue>
#include <list>
#include "eagleeye/basic/spinlock.hpp"
#include <functional>
#include <memory>

namespace eagleeye{

struct ParallelPriorityComp
{
    bool operator() (std::pair<std::shared_ptr<AnySignal>, int> a, std::pair<std::shared_ptr<AnySignal>, int> b) 
    {
        return a.second > b.second; 
    }
};    
class ParallelNode:public AnyNode{
public:
    typedef ParallelNode            Self;
    typedef AnyNode                 Superclass;

    EAGLEEYE_CLASSIDENTITY(ParallelNode);

    /**
     * @brief Construct a new Parallel Node object
     * 
     */
    ParallelNode(int thread_num, std::function<AnyNode*()> generator);
    
    /**
     * @brief Destroy the Parallel Node object
     * 
     */
    virtual ~ParallelNode();

    /**
     * @brief overide setUnitName
     */
	virtual void setUnitName(const char* unit_name);

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    /**
     * @brief (pre/post) exit
     * 
     */
    virtual void preexit();
    virtual void postexit();

    /**
     * @brief refresh data (special design for delay node)
     * 
     */
    virtual void refresh();

    /**
     * @brief reset parallel
     * 
     */
    virtual void reset();

    /**
     * @brief init node
     * 
     */
    virtual void init();

    /**
	 *	@brief get monitor pool of the whole pipeline
	 *	@note traverse the whole pipeline
	 */
	virtual void getPipelineMonitors(std::map<std::string,std::vector<AnyMonitor*>>& pipeline_monitor_pool);

    /**
     * @brief overload
     * @note force tobe updated
     */
    virtual void updateUnitInfo();

    /**
	 * @brief load/save pipeline configure
	 * 
	 * @param node_config 
	 */
	virtual void loadConfigure(std::map<std::string, std::shared_ptr<char>> nodes_config);
	virtual void saveConfigure(std::map<std::string, std::shared_ptr<char>>& nodes_config);

protected:
    /**
     * @brief run subnode parallel
     * 
     * @param thread_id 
     */
    void run(int thread_id);

private:
    ParallelNode(const ParallelNode&);
    void operator=(const ParallelNode&);
    std::vector<AnyNode*> m_run_node;
    std::vector<std::thread> m_run_threads;    
    std::vector<std::priority_queue<std::pair<std::shared_ptr<AnySignal>, int>, std::vector<std::pair<std::shared_ptr<AnySignal>, int>>, ParallelPriorityComp>> m_output_cache;

    unsigned int m_waiting_count;
	std::mutex m_output_mu;	
    std::condition_variable m_output_cond;
    bool m_thread_status;
    bool m_is_ini;
};

}
#endif