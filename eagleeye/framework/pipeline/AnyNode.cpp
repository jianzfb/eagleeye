#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeTime.h"
#include<iostream>

namespace eagleeye{
bool AnyNode::m_saved_resource = false;
std::string AnyNode::m_resource_folder = "/sdcard/";
unsigned long AnyNode::m_pipeline_reset_time = 0;
unsigned long AnyNode::m_pipeline_print_time = 0;
unsigned long AnyNode::m_pipeline_init_time = 0;
unsigned long AnyNode::m_pipeline_exit_time = 0;

AnyNode::AnyNode(const char* unit_name)
		:AnyUnit(unit_name){
	m_updating_flag = false;
	this->m_call_once = true;
	this->m_node_state = 0;
	// this->m_action = 1;	 // "RUN"
	this->m_node_is_satisfied_cond = true;
	this->m_auto_clear = false;

	this->m_reset_flag = false;
	this->m_exit_flag = false;
	this->m_init_flag = false;
	this->m_process_flag = false;
	this->m_get_monitor_flag = false;
	// this->m_feadback_flag = false;
	this->m_finish_flag = false;
	this->m_load_config_flag = false;
	this->m_save_config_flag = false;
	this->m_findin_flag = false;
	this->m_print_flag = false;

	// init timestamp
	this->m_init_timestamp.modified();
	this->m_pipeline_init_time = this->m_init_timestamp.getMTime();
	this->m_init_time = 0;

	// reset timestamp
	this->m_reset_time = 0;
	this->m_pipeline_reset_time = 0;

	// print timestamp
	this->m_print_time = 0;
	this->m_pipeline_print_time = 0;

	// exit timestamp
	this->m_exit_timestamp.modified();
	this->m_pipeline_exit_time = this->m_exit_timestamp.getMTime();
	this->m_exit_time = 0;

	this->m_pipeline = NULL;
	this->m_node_category = COMPUTING;
	this->m_wait_flag = false;
}

AnyNode::~AnyNode()
{	
	std::vector<AnySignal*>::iterator iter,iend(m_output_signals.end());
	for (iter = m_output_signals.begin(); iter != iend; ++iter)
	{
		if ((*iter))
		{
			delete (*iter);
		}
	}
}

void AnyNode::addInputPort(AnySignal* sig)
{	
	m_input_signals.push_back(sig);
	// increment input signal out degree
	sig->incrementOutDegree();
	modified();
}

void AnyNode::setInputPort(AnySignal* sig,int index)
{
	if(m_input_signals[index] != NULL){
		m_input_signals[index]->decrementOutDegree();
	}
	m_input_signals[index] = sig;

	// increment input signal out degree
	if(sig != NULL){
		sig->incrementOutDegree();
	}
	modified();
}

void AnyNode::clearInputPort(int index){
	if(index >= this->m_input_signals.size()){
		return;
	}

	if(m_input_signals[index] != NULL){
		m_input_signals[index]->decrementOutDegree();
	}
	m_input_signals[index] = NULL;
}

void AnyNode::addDependentNode(AnyNode* dependent_node){
	// 忽略依赖节点的输出端口实际含义
	m_input_signals.push_back(dependent_node->getOutputPort(0));
	dependent_node->getOutputPort(0)->incrementOutDegree();
	modified();
}

void AnyNode::removeInputPort(AnySignal* sig)
{
	std::vector<AnySignal*>::iterator iter,iend(m_input_signals.end());
	for (iter = m_input_signals.begin();iter != iend; ++iter)
	{
		if ((*iter) == sig)
		{	
			// decrement input signal out degree
			sig->decrementOutDegree();
			(*iter)=NULL;
		}
	}
	modified();
}
void AnyNode::removeInputPort(int index)
{
	if (index < int(m_input_signals.size()))
	{
		// decrement input signal out degree
		m_input_signals[index]->decrementOutDegree();
		m_input_signals[index]=NULL;
	}
	modified();
}

AnySignal* AnyNode::getInputPort(unsigned int index)
{
	if (index < m_input_signals.size())
	{
		return m_input_signals[index];
	}
	else
	{
		return NULL;
	}
}

const AnySignal* AnyNode::getInputPort(unsigned int index) const 
{
	if (index < m_input_signals.size())
	{
		return m_input_signals[index];
	}
	else
	{
		return NULL;
	}
}

AnySignal* AnyNode::getOutputPort(unsigned int index)
{
	if (index < m_output_signals.size())
	{
		return m_output_signals[index];
	}
	else
	{
		return NULL;
	}
}

const AnySignal* AnyNode::getOutputPort(unsigned int index) const
{
	if (index < m_output_signals.size())
	{
		return m_output_signals[index];
	}
	else
	{
		return NULL;
	}
}

void AnyNode::setOutputPort(AnySignal* sig,int index)
{
	//does this change anything?
	if (index < int(m_output_signals.size()) && sig == m_output_signals[index])
	{
		return;
	}

	//expand array if necessary
	if (index >= int(m_output_signals.size()))
	{
		setNumberOfOutputSignals(index + 1);
	}

	if (m_output_signals[index])
	{
		//delete the old output signal
		m_output_signals[index]->dislinkAnyNode(this,index);
		delete m_output_signals[index];

		m_output_signals[index] = NULL;
	}

	if (sig)
	{
		sig->linkAnyNode(this,index);
	}
	
	//save this sig as output signals
	m_output_signals[index] = sig;

	modified();
}

void AnyNode::setNumberOfOutputSignals(unsigned int outputnum)
{
	if (outputnum != m_output_signals.size())
	{
		std::vector<AnySignal*> output_signals;
		output_signals.resize(outputnum);
		
		std::vector<bool> output_port_states;
		output_port_states.resize(outputnum);

		int old_output_signals_num = m_output_signals.size();

		if (old_output_signals_num < int(outputnum))
		{
			for (int i = 0; i < old_output_signals_num; ++i)
			{
				output_signals[i] = m_output_signals[i];
				output_port_states[i] = m_output_port_state[i];
			}

			for (int i = old_output_signals_num; i < int(outputnum); ++i)
			{
				output_signals[i] = NULL;
				output_port_states[i] = true;
			}
		}
		else
		{
			for (int i = 0; i < int(outputnum); ++i)
			{
				output_signals[i] = m_output_signals[i];
				output_port_states[i] = m_output_port_state[i];
			}

			for (int i = outputnum; i < old_output_signals_num; ++i)
			{
				if (m_output_signals[i])
				{
					m_output_signals[i]->dislinkAnyNode(this,i);
					delete m_output_signals[i];
				}
			}
		}

		m_output_signals = output_signals;
		m_output_port_state = output_port_states;
	}
}
void AnyNode::setNumberOfInputSignals(unsigned int inputnum){
	if (inputnum != m_input_signals.size()){
		std::vector<AnySignal*> input_signals;
		input_signals.resize(inputnum);
		if (inputnum < m_input_signals.size()){
			for (int i = 0; i < int(inputnum); ++i){
				input_signals[i] = m_input_signals[i];
			}

			for(int i=int(inputnum); i<m_input_signals.size(); ++i){
				if(m_input_signals[i] != NULL){
					m_input_signals[i]->decrementOutDegree();
				}
			}
		}
		else{
			for (int i = 0; i < int(m_input_signals.size()); ++i){
				input_signals[i] = m_input_signals[i];
			}
			for (int i = int(m_input_signals.size()); i < int(inputnum); ++i){
				input_signals[i] = NULL;
			}
		}

		m_input_signals = input_signals;
	}
}

void AnyNode::passonNodeInfo(){
	std::vector<AnySignal*>::iterator in_iter,in_iend(m_input_signals.end());
	std::vector<AnySignal*>::iterator out_iter,out_iend(m_output_signals.end());

	for (in_iter = m_input_signals.begin();in_iter != in_iend; ++in_iter){
		for (out_iter = m_output_signals.begin(); out_iter != out_iend; ++out_iter){
			(*out_iter)->copyInfo(*in_iter);
		}
		break;
	}
}

bool AnyNode::start(){
	//update some necessary info, such as basic format or struct of AnySignal(without content),
	//re-assign update time
	std::vector<AnySignal*>::iterator out_iter,out_iend(m_output_signals.end());
	for (out_iter = m_output_signals.begin(); out_iter != out_iend; ++out_iter){
		(*out_iter)->updateUnitInfo();
	}
	for (out_iter = m_output_signals.begin(); out_iter != out_iend; ++out_iter){
		//complement some concrete task, such as generating some data and so on.
		(*out_iter)->processUnitInfo();
	}
	// // feadback
	// std::map<std::string, int> node_state_map;
	// for (out_iter = m_output_signals.begin(); out_iter != out_iend; ++out_iter){
	// 	(*out_iter)->feadback(node_state_map);
	// }
	// return this->isDataHasBeenUpdate();
	bool _is_finish = true;
	for (out_iter = m_output_signals.begin(); out_iter != out_iend; ++out_iter){
		//complement some concrete task, such as generating some data and so on.
		_is_finish = _is_finish & (*out_iter)->finish();
	}

	return _is_finish;
}

void AnyNode::wait(){
	if(this->m_wait_flag){
		return;
	}
	
	//reset pipeline
	this->m_wait_flag = true;
	std::vector<AnySignal*>::iterator in_iter,in_iend(m_input_signals.end());
	for (in_iter = m_input_signals.begin(); in_iter != in_iend; ++in_iter){
		if(*in_iter != NULL){
			(*in_iter)->wait();
		}
	}

	this->m_wait_flag = false;
}

void AnyNode::preset(){
	// 1.step set pipeline reset time
	m_reset_timestamp.modified();
	m_pipeline_reset_time = m_reset_timestamp.getMTime();

	// 2.step reset pipeline
	this->reset();
}

void AnyNode::reset(){
	if(this->m_reset_flag){
		return;
	}
	
	// update reset time
	m_reset_timestamp.modified();
	m_reset_time = m_reset_timestamp.getMTime();

	//reset pipeline
	this->m_reset_flag = true;
	std::vector<AnySignal*>::iterator in_iter,in_iend(m_input_signals.end());
	for (in_iter = m_input_signals.begin(); in_iter != in_iend; ++in_iter){
		if(*in_iter != NULL){
			(*in_iter)->reset();
		}
	}

	this->m_reset_flag = false;
	// log
	EAGLEEYE_LOGD("node %s reset", this->getUnitName());
}

void AnyNode::pprint(){
	// 1.step set pipeline print time
	m_print_timestamp.modified();
	m_pipeline_print_time = m_print_timestamp.getMTime();

	// 2.step reset pipeline
	this->print();
}

void AnyNode::print(){
	if(this->m_print_flag){
		return;
	}

	// update print time
	m_print_timestamp.modified();
	m_print_time = m_print_timestamp.getMTime();

	//print input signal info
	this->m_print_flag = true;
	std::vector<AnySignal*>::iterator in_iter,in_iend(m_input_signals.end());
	for (in_iter = m_input_signals.begin(); in_iter != in_iend; ++in_iter){
		if(*in_iter != NULL){
			(*in_iter)->printUnit();
		}
	}
		
	//print this node info
	printUnit();
	this->m_print_flag = false;
}

void AnyNode::updateUnitInfo()
{
	unsigned long t1,t2;

	//watch out for loops in the pipeline
	//prevent from trapping into dead loop
	if (m_updating_flag)
	{
		modified();
		return;
	}

	//get the parameter update time of of this Node
	//we now wish to set the PipelineMTime of each output signal
	// to the largest of this AnyNode's MTime, all input signal's PipelineMTime
	// , and all input's MTime. We begin with the MTime of this AnyNode.
	t1 = getMTime();

	//Loop through the inputs
	for (unsigned int idx = 0; idx < m_input_signals.size(); ++idx)
	{
		if (m_input_signals[idx])
		{
			AnySignal* input_signal = m_input_signals[idx];

			//propagate update unit info
			//notify the upper unit update
			m_updating_flag = true;
			input_signal->updateUnitInfo();
			m_updating_flag = false;

			//What is the PipelineTime of this input signal? Compare this with
			// our current computation to find the largest one.
			t2=input_signal->getPipelineTime();

			if (t2 > t1)
			{
				t1 = t2;
			}

			//Pipeline Time of this input signal doesn't include the Time of 
			//this input signal itself.
			t2 = input_signal->getMTime();
			if (t2 > t1)
			{
				t1 = t2;
			}
		}
	}

	//judge whether the current node is need to update
	if (t1 > m_node_info_mtime.getMTime())
	{	
		//If the current node is needed to update,
		//we need to re-set the Pipeline Time of all output signals 
		//to force them to update
		for (unsigned int idx = 0;idx < m_output_signals.size(); ++idx)
		{
			AnySignal* output_signal = m_output_signals[idx];

			// modified by Jian 
			// support output port enable/disable
			// if output port disable, output signal don't update time
			// all connected down-stream nodes, wouldn't run			
			if(!this->m_output_port_state[idx]){
				continue;
			}

			output_signal->setPipelineTime(t1);
		}

		//start off passing on node info
		//now we can use the struct info of all m_input_signals 
		passonNodeInfo();
		//record the time that updateUnitInfo() was called
		m_node_info_mtime.modified();
	}

	// this->m_finish_run = false;
}

void AnyNode::processUnitInfo()
{	
	if(m_process_flag){
		return;
	}

	//the upper unit should process unit info firstly.
	std::vector<AnySignal*>::iterator in_iter,in_iend(m_input_signals.end());
	for (in_iter = m_input_signals.begin(); in_iter != in_iend; ++in_iter){
		m_process_flag = true;
		(*in_iter)->processUnitInfo();
		m_process_flag = false;
	}

	if (isNeedProcessed()){
		//execute task
		//now we can use all info of m_input_signals
		EAGLEEYE_LOGD("start run execute node (%s)", getUnitName());
		long start_time = EagleeyeTime::getCurrentTime();
		executeNodeInfo();
		long end_time = EagleeyeTime::getCurrentTime();

		//clear some compute resource
		clearSomething();
		EAGLEEYE_LOGI("finish run node (%s) -- (%s) (%d us)\n",getClassIdentity(),getUnitName(), int(end_time-start_time));

		//all info of m_output_signals has been generated
		//we should change their time stamp
		std::vector<AnySignal*>::iterator out_iter,out_iend(m_output_signals.end());
		for (out_iter = m_output_signals.begin(); out_iter != out_iend; ++out_iter){
			(*out_iter)->signalHasBeenUpdate();
		}
	}
	else{
		//clear some compute resource
		clearSomething();
		EAGLEEYE_LOGI("skip execute node (%s) -- (%s)\n",getClassIdentity(),getUnitName());
	}

	// finish run
	// this->m_finish_run = true;
}

void AnyNode::getPipelineMonitors(std::map<std::string,std::vector<AnyMonitor*>>& pipeline_monitor_pool)
{	
	if(m_get_monitor_flag){
		return;
	}

	if(pipeline_monitor_pool.find(getUnitName()) == pipeline_monitor_pool.end())
	{
		pipeline_monitor_pool[getUnitName()] = m_unit_monitor_pool;
	}

	//traverse the whole pipeline
	m_get_monitor_flag = true;
	std::vector<AnySignal*>::iterator signal_iter,signal_iend(m_input_signals.end());
	for (signal_iter = m_input_signals.begin();signal_iter != signal_iend; ++signal_iter)
	{
		if ((*signal_iter)){
			(*signal_iter)->getPipelineMonitors(pipeline_monitor_pool);
		}
	}
	m_get_monitor_flag = false;
}

void AnyNode::enableOutputPort(int index)
{
	if (index < int(m_output_port_state.size()))
	{
		m_output_port_state[index] = true;
	}
}
void AnyNode::disableOutputPort(int index)
{
	if (index < int(m_output_port_state.size()))
	{	
		m_output_port_state[index] = false;
	}
}

bool AnyNode::selfcheck(){
	// only run once
	if(this->m_call_once){
		// set default unit name
		if (std::string(getUnitName()) == std::string("AnyNode")){
			setUnitName(getClassIdentity());
		}

		//it configure file exists, loading configure parameters
		//it would run only once

		// set false
		this->m_call_once = false;
	}
	return true;
}

void AnyNode::printUnit(){
   EAGLEEYE_LOGI("node id--( %s )  name--( %s ) \n", getClassIdentity(),getUnitName());
}

bool AnyNode::isNeedProcessed(){
	// check all input signals
	bool is_need_processed = true;
	std::vector<AnySignal*>::iterator iter,iend(m_input_signals.end());
	for (iter = m_input_signals.begin();iter != iend; ++iter){
		if(!(*iter)->isPreparedOK()){
			is_need_processed = false;
			break;
		}
	}
	
	std::vector<AnySignal*>::iterator o_iter,o_iend(m_output_signals.end());
	bool is_one_ok = true;
	for (o_iter = m_output_signals.begin();o_iter != o_iend; ++o_iter){
		is_one_ok = is_one_ok | (*o_iter)->isPreparedOK();
	}
	is_need_processed = is_need_processed & is_one_ok;

	// if(this->getNodeAction() != 1){
	// 	is_need_processed = false;
	// }
	return is_need_processed;
}

void AnyNode::clearSomething(){
	if(AnyNode::m_saved_resource || this->m_auto_clear){
		// clear middle resource
		std::vector<AnySignal*>::iterator iter,iend(m_input_signals.end());
		for (iter = m_input_signals.begin();iter != iend; ++iter){
			EAGLEEYE_LOGD("try clear input resource for node %s",this->getUnitName());
			(*iter)->makeempty();
		}
	}
}

void AnyNode::enableSavedResource(){
	AnyNode::m_saved_resource = true;
}

void AnyNode::disableSavedResource(){
	AnyNode::m_saved_resource = false;
} 

void AnyNode::enableAutoClear(){
	this->m_auto_clear = true;
}
void AnyNode::disableAutoClear(){
	this->m_auto_clear = false;
}

// void AnyNode::addFeadbackRule(std::string trigger_node, int trigger_node_state, std::string response_action){
// 	this->m_trigger_node.push_back(trigger_node);
// 	this->m_trigger_node_state.push_back(trigger_node_state);
// 	assert(response_action == "RUN" || response_action == "SKIP");
// 	if(response_action == "RUN"){
// 		this->m_response_actions.push_back(1);
// 	}
// 	else{
// 		this->m_response_actions.push_back(0);
// 	}
// }

// void AnyNode::feadback(std::map<std::string, int>& node_state_map){
// 	if(m_feadback_flag){
// 		return;
// 	}

// 	m_feadback_flag = true;

// 	// change action
// 	for(int index=0; index<this->m_trigger_node.size(); ++index){
// 		if(node_state_map.find(this->m_trigger_node[index]) != node_state_map.end()){
// 			if(node_state_map[this->m_trigger_node[index]] == this->m_trigger_node_state[index]){
// 				this->m_action = this->m_response_actions[index];
// 			}
// 		}
// 	}

// 	// add node state into map
// 	node_state_map[this->getUnitName()] = this->getNodeState();

// 	// continue to top 
// 	std::vector<AnySignal*>::iterator signal_iter,signal_iend(m_input_signals.end());
// 	for (signal_iter = m_input_signals.begin();signal_iter != signal_iend; ++signal_iter){
// 		(*signal_iter)->feadback(node_state_map);
// 	}

// 	m_feadback_flag = false;
// }


bool AnyNode::finish(){
	if(m_finish_flag){
		return true;
	}

	m_finish_flag = true;

	// continue to top 
	std::vector<AnySignal*>::iterator signal_iter,signal_iend(m_input_signals.end());
	bool _is_finish = true;
	for (signal_iter = m_input_signals.begin(); signal_iter != signal_iend; ++signal_iter){
		if(*signal_iter != NULL){
			_is_finish = _is_finish & (*signal_iter)->finish();
		}
	}

	m_finish_flag = false;
	return _is_finish;
}

void AnyNode::loadConfigure(std::map<std::string, std::shared_ptr<char>> nodes_config){
	if(m_load_config_flag){
		return;
	}

	m_load_config_flag = true;
	// 1.step load node configure
	if(nodes_config.find(this->m_unit_name) != nodes_config.end()){
		this->setUnitPara(nodes_config[this->m_unit_name]);
	}

	// 2.step traverse upper nodes
	std::vector<AnySignal*>::iterator in_iter,in_iend(m_input_signals.end());
	for (in_iter = m_input_signals.begin(); in_iter != in_iend; ++in_iter){
		if(*in_iter)
			(*in_iter)->loadConfigure(nodes_config);
	}
	m_load_config_flag = false;
}

void AnyNode::saveConfigure(std::map<std::string, std::shared_ptr<char>>& nodes_config){
	if(m_save_config_flag){
		return;
	}

	m_save_config_flag = true;
	// 1.step get node configure
	if(nodes_config.find(this->m_unit_name) == nodes_config.end()){
		std::shared_ptr<char> node_param;
		this->getUnitPara(node_param);
		nodes_config[this->m_unit_name] = node_param;
	}

	// 2.step traverse upper nodes
	std::vector<AnySignal*>::iterator in_iter,in_iend(m_input_signals.end());
	for (in_iter = m_input_signals.begin(); in_iter != in_iend; ++in_iter){
		if(*in_iter)
			(*in_iter)->saveConfigure(nodes_config);
	}
	m_save_config_flag = false;
}

void AnyNode::exit(){
	if(m_exit_flag){
		return;
	}

	// update exit time
	m_exit_timestamp.modified();
	m_exit_time = m_exit_timestamp.getMTime();

	m_exit_flag = true;
	// process (pre exit)
	this->preexit();

	std::vector<AnySignal*>::iterator in_iter,in_iend(m_input_signals.end());
	for (in_iter = m_input_signals.begin(); in_iter != in_iend; ++in_iter){
		if(*in_iter != NULL){
			(*in_iter)->exit();
		}
	}

	// process (post exit)
	this->postexit();
	m_exit_flag = false;

	// log
	EAGLEEYE_LOGD("node %s exit", this->getUnitName());
}

void AnyNode::init(){
	if(m_init_flag){
		return;
	}

	// update init time
	m_init_timestamp.modified();
	m_init_time = m_init_timestamp.getMTime();

	m_init_flag = true;
	std::vector<AnySignal*>::iterator in_iter,in_iend(m_input_signals.end());
	for (in_iter = m_input_signals.begin(); in_iter != in_iend; ++in_iter){
		if(*in_iter != NULL){
			(*in_iter)->init();	
		}
	}
	m_init_flag = false;

	// log
	EAGLEEYE_LOGD("node %s init", this->getUnitName());
}

void AnyNode::findIn(AnySignal* ptr, std::vector<std::pair<AnyNode*,int>>& ll){
	if(m_findin_flag){
		return;
	}

	m_findin_flag = true;
	for(int i=0; i<m_input_signals.size(); ++i){
		if(ptr == m_input_signals[i]){
			bool ok = true;
			for(int m = 0; m<ll.size(); ++m){
				if(ll[m].first == this){
					ok = false;
					break;
				}
			}

			if(ok){
				ll.push_back(std::pair<AnyNode*, int>(this, i));
			}
		}

		if(m_input_signals[i] != NULL){
			m_input_signals[i]->findIn(ptr, ll);
		}
	}

	m_findin_flag = false;
}

unsigned long AnyNode::getResetTime(){
	return m_reset_time;
}
unsigned long AnyNode::getPipelineResetTime(){
	return m_pipeline_reset_time;
}

unsigned long AnyNode::getPrintTime(){
	return this->m_print_time;
}

unsigned long AnyNode::getPipelinePrintTime(){
	return m_pipeline_print_time;
}

unsigned long AnyNode::getInitTime(){
	return this->m_init_time;
}

unsigned long AnyNode::getPipelineInitTime(){
	return this->m_pipeline_init_time;
}

unsigned long AnyNode::getExitTime(){
	return this->m_exit_time;
}

unsigned long AnyNode::getPipelineExitTime(){
	return this->m_pipeline_exit_time;
}

std::string AnyNode::resourceFolder(){
	return AnyNode::m_resource_folder;
}

void AnyNode::setResourceFolder(std::string folder){
	AnyNode::m_resource_folder = folder;
}
}

