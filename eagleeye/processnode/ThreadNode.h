#ifndef _EAGLEEYE_THREADNODE_H_
#define _EAGLEEYE_THREADNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include <vector>
#include <thread>
#include <deque>
#include <queue>
#include <list>
#include "eagleeye/basic/spinlock.hpp"
#include <functional>
#include <memory>
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"


namespace eagleeye{
class ThreadNode:public AnyNode, DynamicNodeCreator<ThreadNode>{
public:
    typedef ThreadNode                  Self;
    typedef AnyNode                     Superclass;

    EAGLEEYE_CLASSIDENTITY(ThreadNode);

    /**
     * @brief Construct a new Parallel Node object
     * 
     */
    ThreadNode(std::function<AnyNode*()> generator=nullptr);
    
    /**
     * @brief Destroy the Parallel Node object
     * 
     */
    virtual ~ThreadNode();

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
     * @brief exit node
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
     * @brief reset ThreadNode
     * 
     */
    virtual void reset();

    /**
     * @brief wait
     */ 
    virtual void wait();

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
    /**
     * @brief run subnode in thread
     * 
     */
    void run();

private:
    ThreadNode(const ThreadNode&);
    void operator=(const ThreadNode&);

    AnyNode* m_run_node;
    std::thread m_run_thread;

    std::deque<std::vector<std::shared_ptr<AnySignal>>> m_input_cache;
    std::deque<std::vector<std::shared_ptr<AnySignal>>> m_output_list;

    std::mutex m_input_mu;
    std::mutex m_output_mu;
    std::condition_variable m_input_cond;
    volatile bool m_thread_status;

    int m_receive_num;
    int m_finish_num;
};    
}
#endif