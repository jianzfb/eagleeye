#ifndef _EAGLEEYE_ASYNNODE_H_
#define _EAGLEEYE_ASYNNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/processnode/SubPipeline.h"
#include <vector>
#include <thread>
#include <deque>
#include <queue>
#include <list>
#include "eagleeye/basic/spinlock.hpp"
#include <functional>
#include <memory>

namespace eagleeye{

struct AsynMetaData{
    AsynMetaData(int a, int b):timestamp(a),round(b){}
    int timestamp;
    int round;
};

class AsynNode:public AnyNode{
public:
    typedef AsynNode                Self;
    typedef AnyNode                 Superclass;

    EAGLEEYE_CLASSIDENTITY(AsynNode);

    /**
     * @brief Construct a new Parallel Node object
     * 
     */
    AsynNode(int thread_num, std::function<AnyNode*()> generator, int input_queue_size=1, int output_queue_size=1);
    
    /**
     * @brief Destroy the Parallel Node object
     * 
     */
    virtual ~AsynNode();

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    /**
     * @brief exit node
     * 
     */
    virtual void exit();

    /**
     * @brief refresh data (special design for delay node)
     * 
     */
    virtual void refresh();

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
    /**
     * @brief run subnode parallel
     * 
     * @param thread_id 
     */
    void run(int thread_id);

private:
    AsynNode(const AsynNode&);
    void operator=(const AsynNode&);

    std::vector<AnyNode*> m_run_node;
    std::vector<std::thread> m_run_threads;    

    std::deque<std::pair<std::shared_ptr<AnySignal>, AsynMetaData>> m_input_cache;
    std::list<std::pair<std::shared_ptr<AnySignal>, AsynMetaData>> m_output_list;

    unsigned int m_process_count;
    std::mutex m_input_mu;
	std::mutex m_output_mu;	
    std::condition_variable m_input_cond;
    bool m_thread_status;
    int m_input_queue_size;
    int m_output_queue_size;
    int m_round;

    bool m_reset_flag;
};
}
#endif