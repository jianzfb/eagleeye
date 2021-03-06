#ifndef _EAGLEEYE_AUTONODE_H_
#define _EAGLEEYE_AUTONODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/spinlock.hpp"
#include <vector>
#include <map>
#include <memory>


namespace eagleeye{
class AutoNode:public AnyNode{
public:
    typedef AutoNode                Self;
    typedef AnyNode                 Superclass;
    EAGLEEYE_CLASSIDENTITY(AutoNode);

    AutoNode(std::function<AnyNode*()> generator);
    virtual ~AutoNode();

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
     * @brief reset node
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
	 * @brief load/save pipeline configure
	 * 
	 * @param node_config 
	 */
	virtual void loadConfigure(std::map<std::string, std::shared_ptr<char>> nodes_config);
	virtual void saveConfigure(std::map<std::string, std::shared_ptr<char>>& nodes_config);

    /**
     * @brief overload
     * @note force tobe updated
     */
    virtual void updateUnitInfo();

protected:
    /**
     * @brief run in independent thread
     * 
     * @param thread_id 
     */
    void run();

private:
    AutoNode(const AutoNode&);
    void operator=(const AutoNode&);

    AnyNode* m_auto_node;
    bool m_thread_status;
    std::thread m_auto_thread;
    bool m_is_ini;
};
}
#endif