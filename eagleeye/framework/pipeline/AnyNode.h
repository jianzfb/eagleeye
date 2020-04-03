#ifndef _ANYNODE_H_
#define _ANYNODE_H_

#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyUnit.h"
#include "eagleeye/framework/pipeline/AnySignal.h"
#include "eagleeye/framework/EagleeyeTimeStamp.h"
#include <map>
#include <vector>

namespace eagleeye
{
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
	 *	@brief start pipeline mechanism
	 */
	bool start();

	/**
	 * @brief reset pipeline
	 * 
	 */
	virtual void reset();

	/**
	 *	@brief print pipeline info
	 */
	void print();

	/**
	 *	@brief some functions about input signals
	 */
	virtual void addInputPort(AnySignal* sig);
	virtual void setInputPort(AnySignal* sig,int index=0);
	virtual void removeInputPort(AnySignal* sig);
	virtual void removeInputPort(int index);

	virtual AnySignal* getInputPort(unsigned int index=0);
	virtual const AnySignal* getInputPort(unsigned int index=0) const;

	/**
	 *	@brief get output signal
	 */
	virtual AnySignal* getOutputPort(unsigned int index=0);
	virtual const AnySignal* getOutputPort(unsigned int index=0) const;
	
	virtual void setOutputPort(AnySignal* sig,int index=0);
	
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
	 *	@brief reset the pipeline
	 *	@note Force the whole pipeline to update
	 */
	virtual void resetPipeline();

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

	/**
	 * @brief Get the Node Action object
	 * SKIP(0)/RUN(1)
	 * @return std::string 
	 */
	int getNodeAction(){return this->m_action;};

	/**
	 * @brief Get/Set is success state
	 * 
	 * @return true 
	 * @return false 
	 */
	bool getIsSatisfiedCondition(){return m_node_is_satisfied_cond;};
	void setIsSatisfiedCondition(bool status){this->m_node_is_satisfied_cond = status;};

	/**
	 * @brief Set the Node Action object
	 * 
	 * @param action 
	 */
	void setNodeAction(int action){this->m_action = action;}

	/**
	 * @brief add feadback rule action
	 * 
	 * @param trigger_node 
	 * @param trigger_node_state 
	 * @param action 
	 */
	void addFeadbackRule(std::string trigger_node, int trigger_node_state, std::string response_action);

	/**
	 * @brief from bottom to top, feadback
	 * 
	 */
	virtual void feadback(std::map<std::string, int>& node_state_map);

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

	/**
	 * @brief whether data has been update
	 * 
	 * @return true 
	 * @return false 
	 */
	bool isDataHasBeenUpdate(){return this->m_finish_run;};

	/**
	 * @brief load/save pipeline configure
	 * 
	 * @param node_config 
	 */
	void loadConfigure(std::map<std::string, std::shared_ptr<char>> nodes_config);
	void saveConfigure(std::map<std::string, std::shared_ptr<char>>& nodes_config);

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

	/**
	 * @brief after executing node, node state maybe change. which would influence other node action.
	 * 
	 */
	int m_node_state;
	bool m_node_is_satisfied_cond;
	int m_action;

	std::vector<std::string> m_trigger_node;
	std::vector<int> m_trigger_node_state;
	std::vector<int> m_response_actions;

	bool m_finish_run;

private:
	AnyNode(const AnyNode&);
	void operator=(const AnyNode&);

	bool m_reset_flag;
	bool m_exit_flag;
	bool m_init_flag;
	bool m_process_flag;
	bool m_get_monitor_flag;
	bool m_feadback_flag;
	bool m_load_config_flag;
	bool m_save_config_flag;
};
}


#endif
