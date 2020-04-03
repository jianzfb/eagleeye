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
class ParallelNode:public AnyNode{
public:
    typedef ParallelNode            Self;
    typedef AnyNode                 Superclass;

    EAGLEEYE_CLASSIDENTITY(ParallelNode);

    /**
     * @brief Construct a new Parallel Node object
     * 
     */
    ParallelNode(int delay_time, int thread_num, std::function<AnyNode*()> generator);
    
    /**
     * @brief Destroy the Parallel Node object
     * 
     */
    virtual ~ParallelNode();

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    /**
     * @brief exit node
     * 
     */
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
     * @brief Set/Get the Delay Time object
     * 
     * @param delay_time 
     */
    void setDelayTime(int delay_time);
    void getDelayTime(int& delay_time);

    /**
     * @brief feadback after run
     * 
     * @param node_state_map 
     */
    void feadback(std::map<std::string, int>& node_state_map);

    /**
	 *	@brief get monitor pool of the whole pipeline
	 *	@note traverse the whole pipeline
	 */
	virtual void getPipelineMonitors(std::map<std::string,std::vector<AnyMonitor*>>& pipeline_monitor_pool);

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
    std::queue<std::pair<std::shared_ptr<AnySignal>, int>> m_input_cache;
    std::list<std::pair<std::shared_ptr<AnySignal>, int>> m_output_cache;

    unsigned int m_process_count;
    unsigned int m_waiting_count;
    std::mutex m_input_mu;
	std::mutex m_output_mu;	
    std::condition_variable m_input_cond;
    std::condition_variable m_output_cond;
    bool m_thread_status;

};

}
#endif