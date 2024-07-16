#ifndef _EAGLEEYE_FRAMESYNCNODE_H_
#define _EAGLEEYE_FRAMESYNCNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"
#include <vector>
#include <queue>

namespace eagleeye{
class FrameSyncNode: public AnyNode, DynamicNodeCreator<FrameSyncNode>{
public:
    typedef FrameSyncNode               Self;
    typedef AnyNode                     Superclass;

    /**
	 *	@brief Get class identity
	 */
    EAGLEEYE_CLASSIDENTITY(FrameSyncNode);

    /**
     *  @brief constructor/destructor
     */
    FrameSyncNode();
    virtual ~FrameSyncNode();
    virtual void processUnitInfo() override;

    virtual void executeNodeInfo();

    void addInputPort(AnySignal* sig);
    void setInputPort(AnySignal* sig,int index);
    
    /**
     * @brief (pre/post) exit
     * 
     */
    virtual void preexit();
    virtual void postexit();

    virtual void processUnitInfo() override;
protected:
    /**
     * @brief run in independent thread
     * 
     * @param thread_id 
     */
    void run();

private:
    FrameSyncNode(const FrameSyncNode&);
    void operator=(const FrameSyncNode&);  

    bool m_thread_status;
    bool m_is_ini;
    std::thread m_auto_thread;
    std::vector<std::queue<Matrix<Array<unsigned char,3>>>> m_frame_cache_queue;
    std::vector<std::queue<MetaData>> m_meta_cache_queue;
    int m_max_cache_frame_num;
    
    std::mutex m_mu;
	std::condition_variable m_cond;

    double m_sync_time_delta;
};
}
#endif