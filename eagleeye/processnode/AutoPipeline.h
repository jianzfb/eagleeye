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

    AutoPipeline(std::function<AnyPipeline*()> generator, std::vector<std::pair<std::string, int>> export_node, int queue_size=1,  bool get_then_auto_remove=true, bool set_then_auto_remove=true, bool copy_input=true);
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
	virtual void setCallback(std::string name, std::function<void(AnyNode*, std::vector<AnySignal*>)> callback);

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

    /**
     * @brief enable/disable auto stop(根据Meta信息自动结束)
     */
    void enableAutoStop();
    void disableAutoStop();

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

    std::vector<double> m_last_timestamp;
    bool m_enable_auto_stop;
    bool m_copy_input;

    bool m_get_then_auto_remove;
    bool m_set_then_auto_remove;

    bool m_is_first_run;
}; 
}
#endif