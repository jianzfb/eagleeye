#include "eagleeye/framework/pipeline/AnySignal.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/common/EagleeyeLog.h"

namespace eagleeye
{
AnySignal::AnySignal(const char* unit_name, const char* signal_type, const char* signal_target)
:AnyUnit(unit_name)
{
	m_pipeline_time=0;
	m_link_index=-1;
	m_link_node=NULL;

	m_signal_type = signal_type;
	m_signal_target = signal_target;
	this->m_prepared_ok = false;
	this->m_delay_time = 0;
	this->m_out_degree = 0;

	// set support signal type and target
	mSupportSignalType[EAGLEEYE_SIGNAL_IMAGE] = "IMAGE";
	mSupportSignalType[EAGLEEYE_SIGNAL_RECT] = "RECT";
	mSupportSignalType[EAGLEEYE_SIGNAL_LINE] = "LINE";
	mSupportSignalType[EAGLEEYE_SIGNAL_POINT] = "POINT";
	mSupportSignalType[EAGLEEYE_SIGNAL_STRING] = "STRING";
	mSupportSignalType[EAGLEEYE_SIGNAL_MASK] = "MASK";

	mSupportSignalTarget[EAGLEEYE_CAPTURE_STILL_IMAGE] = "CAPTURE_STILL_IMAGE";
	mSupportSignalTarget[EAGLEEYE_PHOTO_GALLERY_IMAGE] = "PHOTO_GALLERY_IMAGE";
	mSupportSignalTarget[EAGLEEYE_CAPTURE_PREVIEW_IMAGE] = "CAPTURE_PREVIEW_IMAGE";
	mSupportSignalTarget[EAGLEEYE_CAPTURE_CLICK] = "CAPTURE_CLICK";
	mSupportSignalTarget[EAGLEEYE_CAPTURE_LINE] = "CAPTURE_LINE";
	mSupportSignalTarget[EAGLEEYE_CAPTURE_RECT] = "CAPTURE_RECT";
	mSupportSignalTarget[EAGLEEYE_CAPTURE_POINT] = "CAPTURE_POINT";
	mSupportSignalTarget[EAGLEEYE_CAPTURE_MASK] = "CAPTURE_MASK";
	mSupportSignalTarget[EAGLEEYE_CAPTURE_VIDEO_IMAGE] = "CAPTURE_VIDEO_IMAGE";
	this->m_signal_target = "UNDEFINED";
}
AnySignal::~AnySignal()
{

}

unsigned long AnySignal::getPipelineTime()
{
	return m_pipeline_time;
}

void AnySignal::setPipelineTime(unsigned long time)
{
	m_pipeline_time=time;
}

void AnySignal::linkAnyNode(AnyNode* node,int index)
{
	m_link_node=node;
	m_link_index=index;
}
void AnySignal::dislinkAnyNode(AnyNode* node,int index)
{
	if (m_link_node==node&&m_link_index==index)
	{
		m_link_node=NULL;
		m_link_index=-1;
	}
}

void AnySignal::updateUnitInfo(){
	//if selfcheck return false, it illustrates that
	//there is no necessary to update this node.
	if (m_link_node){
		if (!m_link_node->selfcheck()){
			// node selfcheck fail
			this->m_prepared_ok = false;			
           	EAGLEEYE_LOGD("%s failed to selfcheck\n",m_link_node->getUnitName());
		}
		else if(m_link_node->getNodeBlockStatus()){
			// block pipeline 
			this->m_prepared_ok = false;
		}
		else{
			// node computation OK
			this->m_prepared_ok = true;
		}

		m_link_node->updateUnitInfo();
	}
	else{
		this->m_prepared_ok = true;
	}
}

void AnySignal::processUnitInfo()
{	
	if ((getMTime()<m_pipeline_time && m_link_node) || (m_link_node && EagleeyeTimeStamp::isCrossBorder())){
		m_link_node->processUnitInfo();
	}

	if(!m_link_node && EagleeyeTimeStamp::isCrossBorder()){
		EagleeyeTimeStamp::resetCrossBorder();
	}
}

void AnySignal::signalHasBeenUpdate()
{
	modified();
}

void AnySignal::getPipelineMonitors(std::map<std::string,std::vector<AnyMonitor*>>& pipeline_monitor_pool)
{
	if (m_link_node){
		m_link_node->getPipelineMonitors(pipeline_monitor_pool);
	}
}

void AnySignal::getPipelineInputs(std::map<std::string,AnyNode*>& pipeline_inputs){
	if(m_link_node){
		this->m_link_node->getPipelineInputs(pipeline_inputs);
	}
}

void AnySignal::printUnit(){
	if (m_link_node){
		m_link_node->print();
       EAGLEEYE_LOGD("signal (%s) -- (%s) \n",getClassIdentity(),getUnitName());
       EAGLEEYE_LOGD("link node (%s) -- (%s) \n",m_link_node->getClassIdentity(),m_link_node->getUnitName());
	}
}

void AnySignal::reset(){
	if(m_link_node){
		this->m_link_node->reset();
	}
}

void AnySignal::setSignalType(const char* type){
	this->m_signal_type = type;
}

void AnySignal::setSignalType(SignalType type){
	if(this->mSupportSignalType.find(type) == this->mSupportSignalType.end()){
		EAGLEEYE_LOGE("dont support type %d",(int)type);
		return;
	}
	this->m_signal_type = this->mSupportSignalType[type];
}

const char* AnySignal::getSignalType(){
	return this->m_signal_type.c_str();
}

void AnySignal::setSignalTarget(const char* target){
	this->m_signal_target = target;
}

void AnySignal::setSignalTarget(SignalTarget target){
	if(this->mSupportSignalTarget.find(target) == this->mSupportSignalTarget.end()){
		EAGLEEYE_LOGE("dont support target %d",(int)target);
		return;
	}
	this->m_signal_target = this->mSupportSignalTarget[target];
}

const char* AnySignal::getSignalTarget(){
	return this->m_signal_target.c_str();
}

void AnySignal::feadback(std::map<std::string, int>& node_state_map){
	if (m_link_node){
		m_link_node->feadback(node_state_map);
	}
}

void AnySignal::loadConfigure(std::map<std::string, std::shared_ptr<char>> nodes_config){
	if(m_link_node){
		m_link_node->loadConfigure(nodes_config);
	}
}

void AnySignal::saveConfigure(std::map<std::string, std::shared_ptr<char>>& nodes_config){
	if(m_link_node){
		m_link_node->saveConfigure(nodes_config);
	}
}

}
