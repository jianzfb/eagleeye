#ifndef _EAGLEEYE_AUTOPIPELINE_H_
#define _EAGLEEYE_AUTOPIPELINE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/AnyPipeline.h"
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <thread>


namespace eagleeye{
class AutoPipeline:public AnyNode{
public:
    typedef AutoPipeline                Self;
    typedef AnyNode                     Superclass;
    EAGLEEYE_CLASSIDENTITY(AutoPipeline);

    AutoPipeline(std::function<AnyPipeline*()> generator, std::vector<std::pair<std::string, int>> export_node, int queue_size=1,  bool get_then_auto_remove=true);
    virtual ~AutoPipeline();

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    /**
     * @brief overide setUnitName
     */
	virtual void setUnitName(const char* unit_name);

    virtual void exit() override;
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
     * @brief overload
     * @note force tobe updated
     */
    virtual void updateUnitInfo();

    /**
     * @brief set call back
     */
	virtual void setCallback(std::function<void(AnyNode*, std::vector<AnySignal*>)> callback);

    /**
     * @brief get inner delegation instance
     */
    AnyPipeline* getInnerIns(){return m_auto_pipeline;};

    /**
     * @brief 常驻标记
     */
    void setPersistent(bool flag){this->m_persistent_flag=flag;};

    /**
     * @brief 检查是否停止运行
     */
    virtual bool stop(bool block=false, bool force=false);

protected:
    void run();

private:
    AutoPipeline(const AutoPipeline&);
    void operator=(const AutoPipeline&);

    bool m_is_ini;
    AnyPipeline* m_auto_pipeline;
    std::vector<std::pair<std::string, int>> m_pipeline_node;
    std::thread m_auto_thread;
    bool m_thread_status;
    bool m_persistent_flag;
    std::function<void(AnyNode*, std::vector<AnySignal*>)> m_callback;

    std::vector<AnySignal*> m_cache_input;
    std::vector<double> m_last_timestamp;
}; 
}
#endif