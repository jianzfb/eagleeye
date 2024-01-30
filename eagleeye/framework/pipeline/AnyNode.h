#ifndef _ANYNODE_H_
#define _ANYNODE_H_

#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyUnit.h"
#include "eagleeye/framework/pipeline/AnySignal.h"
#include "eagleeye/framework/EagleeyeTimeStamp.h"
#include <map>
#include <vector>
#include <thread>

namespace eagleeye
{
enum NodeCategory{
	PLACEHOLDER,
	COMPUTING,
	RENDER
};

class AnyPipeline;
class EAGLEEYE_API AnyNode:public AnyUnit
{
public:
	/**
	 *	@brief define some basic type
	 *	@note you must do these
	 */
	typedef AnyNode								Self;
	typedef AnyUnit								Superclass;

	AnyNode(const char* unit_name = "AnyNode");
	virtual ~AnyNode();

	/**
	 *	@brief Get class identity
	 */
	EAGLEEYE_CLASSIDENTITY(AnyNode);

	/**
	* @brief create basic info
	*/
	virtual void config(std::function<std::vector<AnyNode*>()> generator){};

	/**
	 *	@brief start pipeline run
	 */
	bool start();

	/**
	 * 	@brief waiting pipeline run
	 */ 
	virtual void wait();

	/**
	 *	@brief reset the pipeline
	 */
	void preset();

	/**
	 * @brief print the pipeline
	 * 
	 */
	void pprint();

	/**
	 * @brief reset node
	 * 
	 */
	virtual void reset();

	/**
	 *	@brief print node info
	 */
	virtual void print();

	/**
	 *	@brief some functions about input signals
	 */
	virtual void addInputPort(AnySignal* sig);
	virtual void setInputPort(AnySignal* sig,int index=0);
	virtual void removeInputPort(AnySignal* sig);
	virtual void removeInputPort(int index);
	void clearInputPort(int index);

	/**
	 * @brief add dependent node(only for order compute)
	 * 
	 * @param dependent_node
	 */
	void addDependentNode(AnyNode* dependent_node);

	virtual AnySignal* getInputPort(unsigned int index=0);
	virtual const AnySignal* getInputPort(unsigned int index=0) const;

	/**
	 *	@brief get output signal
	 */
	virtual AnySignal* getOutputPort(unsigned int index=0);
	virtual const AnySignal* getOutputPort(unsigned int index=0) const;
	
	virtual void setOutputPort(AnySignal* sig,int index=0);
	
	/**
	 * @brief Get the Recurrent Output Port object
	 * 
	 * @param index 
	 * @return AnySignal* 
	 */
	virtual AnySignal* getRecurrentOutputPort(unsigned int index=0){return NULL;}

	/**
	 *	@brief set/get the number of output signals and input signals
	 */
	void setNumberOfOutputSignals(unsigned int outputnum);
	int getNumberOfOutputSignals(){return int(m_output_signals.size());};
	void setNumberOfInputSignals(unsigned int inputnum);
	int getNumberOfInputSignals(){return int(m_input_signals.size());};
	
	/**
	 *	@brief enable/disable output port
	 */
	void enableOutputPort(int index);
	void disableOutputPort(int index);

	/**
	 *	@brief Help passing on some info hold by signal, such as
	 *	struct info, size or other things.
	 *	@note If you want to achieve some complex functions.
	 *	The subclass should implement this function.
	 */
	virtual void passonNodeInfo();

	/**
	 *	@brief execute some concrete task
	 *	@note if necessary, the subclass should overload this
	 *	function.
	 */
	virtual void executeNodeInfo(){};

	/**
	 * @brief call before executeNodeInfo()
	 * 
	 */
	virtual void executeNodeInfoBefore(){};

	/**
	 * @brief call after executeNodeInfo()
	 * 
	 */
	virtual void executeNodeInfoAfter(){};

	/**
	 *	@brief update unit info, which lead to update pipeline
	 */
	virtual void updateUnitInfo();

	/**
	 *	@brief complete some concrete task, such as generating 
	 *	some data
	 */
	virtual void processUnitInfo();

	/**
	 *	@brief node self check
	 *	@note check whether some preliminary parameters have been set. This 
	 *	function would be called before "updateUnitInfo()" automatically.
	 *	Please notice that if "selfcheck" is failed, current node and all
	 * 	succeed nodes wouldnt be run.
	 */
	virtual bool selfcheck();

	/**
	 *	@brief get monitor pool of the whole pipeline
	 *	@note traverse the whole pipeline
	 */
	virtual void getPipelineMonitors(std::map<std::string,std::vector<AnyMonitor*>>& pipeline_monitor_pool);

	/**
	 *	@brief print this node info
	 */
	virtual void printUnit();

	/**
	 * @brief enable save resource mode
	 * 
	 */
	static void enableSavedResource();

	/**
	 * @brief disable save resource mode
	 * 
	 */
	static void disableSavedResource();

	/**
	 * @brief enable auto clear input data
	 * 
	 */
	void enableAutoClear();

	/**
	 * @brief disable auto clear input data
	 * 
	 */
	void disableAutoClear();

	/**
	 * @brief Set the Node State object
	 * 
	 * @param node_state 
	 */
	void setNodeState(int node_state){this->m_node_state = node_state;}

	/**
	 * @brief Get the Node State object
	 * 
	 * @return int 
	 */
	int getNodeState(){return this->m_node_state;}

	// /**
	//  * @brief Get the Node Action object
	//  * SKIP(0)/RUN(1)
	//  * @return std::string 
	//  */
	// int getNodeAction(){return this->m_action;};

	/**
	 * @brief Get/Set is success state
	 * 
	 * @return true 
	 * @return false 
	 */
	bool getIsSatisfiedCondition(){return m_node_is_satisfied_cond;};
	void setIsSatisfiedCondition(bool status){this->m_node_is_satisfied_cond = status;};

	// /**
	//  * @brief Set the Node Action object
	//  * 
	//  * @param action 
	//  */
	// void setNodeAction(int action){this->m_action = action;}

	// /**
	//  * @brief add feadback rule action
	//  * 
	//  * @param trigger_node 
	//  * @param trigger_node_state 
	//  * @param action 
	//  */
	// void addFeadbackRule(std::string trigger_node, int trigger_node_state, std::string response_action);

	// /**
	//  * @brief from bottom to top, feadback
	//  * 
	//  */
	// virtual void feadback(std::map<std::string, int>& node_state_map);

	virtual bool finish();

	/**
	 * @brief exit node
	 * 
	 */
	virtual void exit();
	virtual void preexit(){};
	virtual void postexit(){};

	/**
	 * @brief initialize node
	 * 
	 */
	virtual void init();

	/**
	 * @brief refresh data (special design for delay node)
	 * after refresh, force node output to update
	 */
	virtual void refresh(){}

	// /**
	//  * @brief whether data has been update
	//  * 
	//  * @return true 
	//  * @return false 
	//  */
	// bool isDataHasBeenUpdate(){return this->m_finish_run;};

	/**
	 * @brief load/save pipeline configure
	 * 
	 * @param node_config 
	 */
	virtual void loadConfigure(std::map<std::string, std::shared_ptr<char>> nodes_config);
	virtual void saveConfigure(std::map<std::string, std::shared_ptr<char>>& nodes_config);

	/**
	 * @brief find input signal
	 * 
	 * @param ptr 
	 * @param ll 
	 */
	void findIn(AnySignal* ptr, std::vector<std::pair<AnyNode*,int>>& ll);

	/**
	 * @brief Get the Reset Time object
	 * 
	 * @return unsigned long 
	 */
	unsigned long getResetTime();
	unsigned long getPipelineResetTime();

	/**
	 * @brief Get the Print Time object
	 * 
	 * @return unsigned long 
	 */
	unsigned long getPrintTime();
	unsigned long getPipelinePrintTime();

	/**
	 * @brief Get the Init Time object
	 * 
	 * @return unsigned long 
	 */
	unsigned long getInitTime();
	unsigned long getPipelineInitTime();

	/**
	 * @brief Get the Exit Time object
	 * 
	 * @return unsigned long 
	 */
	unsigned long getExitTime();
	unsigned long getPipelineExitTime();

	/**
	 * @brief set/get pipeline
	 */ 
	AnyPipeline* getPipeline(){return this->m_pipeline;};
	void setPipeline(AnyPipeline* pipeline){this->m_pipeline = pipeline;};

	NodeCategory getNodeCategory(){return this->m_node_category;};
	void setNodeCategory(NodeCategory node_category){this->m_node_category = node_category;};

	/**
	 * @brief get resource folder
	 * 
	 * @return std::string 
	 */
	std::string resourceFolder();

	/**
	 * @brief set callback
	 */
	virtual void setCallback(std::function<void(AnyNode*, std::vector<AnySignal*>)> callback){};

	/**
	 * @brief Set the Resource Folder object
	 * 
	 * @param folder 
	 */
	static void setResourceFolder(std::string folder);

	/**
	 * @brief asyn destroy node
	 */
	static void asnyDestroy(AnyNode* cur_node);

	/**
	 * @brief register aux node (跳过管线框架，直接获取额外节点)
	 */
	void registerAuxNode(AnyNode* aux_node);
	std::vector<AnyNode*> getAuxNode(){return this->m_aux_nodes;}

	/**
	 * @brief set/get folder
	 */
    virtual void setFolder(std::string folder){};
    virtual void getFolder(std::string& folder){};

	/**
	 * @brief stop node (AutoNode, AutoPipeline有效)
	 */
	virtual bool stop(bool block=false, bool force=false){};

protected:
	/**
	 *	@brief make one output signal
	 *	@note If you want a special signal, you have to
	 *	overload this function in the subclass
	 */
	virtual AnySignal* makeOutputSignal(){return NULL;};

	/**
	 *	@brief After processing unit info, perhaps we would clear some
	 *	compute resource
	 */
	virtual void clearSomething();

	/**
	 *	@brief According to some outside info, get the flag whether it \n
	 *	needs to be processed.
	 *	@note Sometimes, some outside info would influence the node's behavior.
	 */
	virtual bool isNeedProcessed();

	std::vector<AnySignal*> m_input_signals;
	std::vector<AnySignal*> m_output_signals;
	std::vector<bool> m_output_port_state;

	EagleeyeTimeStamp m_node_info_mtime;
	bool m_updating_flag;
	bool m_call_once;
	static bool m_saved_resource;
	static bool m_run_in_last_round;

	bool m_auto_clear;

	/**
	 * @brief after executing node, node state maybe change. which would influence other node action.
	 * 
	 */
	int m_node_state;
	bool m_node_is_satisfied_cond;
	// int m_action;

	// std::vector<std::string> m_trigger_node;
	// std::vector<int> m_trigger_node_state;
	// std::vector<int> m_response_actions;

	// bool m_finish_run;
	EagleeyeTimeStamp m_reset_timestamp;
	unsigned long m_reset_time;
	static unsigned long m_pipeline_reset_time;

	EagleeyeTimeStamp m_print_timestamp;
	unsigned long m_print_time;
	static unsigned long m_pipeline_print_time;

	EagleeyeTimeStamp m_init_timestamp;
	unsigned long m_init_time;
	static unsigned long m_pipeline_init_time;

	EagleeyeTimeStamp m_exit_timestamp;
	unsigned long m_exit_time;
	static unsigned long m_pipeline_exit_time;

	NodeCategory m_node_category;

	std::vector<AnyNode*> m_aux_nodes;

private:
	AnyNode(const AnyNode&);
	void operator=(const AnyNode&);

	bool m_reset_flag;			// prevent recurrent call (reset)
	bool m_exit_flag;			// prevent recurrent call (exit)
	bool m_init_flag;			// prevent recurrent call (init)
	bool m_process_flag;		// prevent recurrent call (process)
	bool m_wait_flag;			// prevent recurrent call (wait)

	bool m_init_once;			// only init once flag

	bool m_get_monitor_flag;	// ...
	// bool m_feadback_flag;		// ...
	bool m_finish_flag;
	bool m_load_config_flag;	// ...
	bool m_save_config_flag;	// ...
	bool m_print_flag;			// ...
	bool m_findin_flag;			// ...

	AnyPipeline* m_pipeline;
	static std::string m_resource_folder;
};
}


#endif
