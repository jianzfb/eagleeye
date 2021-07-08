#include "eagleeye/framework/pipeline/AnySignal.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/common/EagleeyeLog.h"

namespace eagleeye
{
const char* SignalTypeName[]={
	"EAGLEEYE_SIGNAL_IMAGE",
	"EAGLEEYE_SIGNAL_RECT",
	"EAGLEEYE_SIGNAL_LINE",
	"EAGLEEYE_SIGNAL_POINT",
	"EAGLEEYE_SIGNAL_STRING",
	"EAGLEEYE_SIGNAL_MASK",
	"EAGLEEYE_SIGNAL_FILE",
	"EAGLEEYE_SIGNAL_TEXT",
	"EAGLEEYE_SIGNAL_MODEL",
	"EAGLEEYE_SIGNAL_LANDMARK",
	"EAGLEEYE_SIGNAL_STATE",
	"EAGLEEYE_SIGNAL_SWITCH",
	"EAGLEEYE_SIGNAL_RGB_IMAGE",
	"EAGLEEYE_SIGNAL_RGBA_IMAGE",
	"EAGLEEYE_SIGNAL_BGR_IMAGE",
	"EAGLEEYE_SIGNAL_BGRA_IMAGE",
	"EAGLEEYE_SIGNAL_GRAY_IMAGE",
	"EAGLEEYE_SIGNAL_YUV_IMAGE",
	"EAGLEEYE_SIGNAL_TEXTURE",
	"EAGLEEYE_SIGNAL_DET",
	"EAGLEEYE_SIGNAL_TRACKING",
	"EAGLEEYE_SIGNAL_POS_2D"
};

const char* SignalTargetName[]={
	"CAPTURE_STILL_IMAGE",
	"PHOTO_GALLERY_IMAGE",
	"CAPTURE_PREVIEW_IMAGE",
	"CAPTURE_CLICK",
	"CAPTURE_LINE",
	"CAPTURE_RECT",
	"CAPTURE_POINT",
	"CAPTURE_MASK",
	"CAPTURE_VIDEO_IMAGE"
};

AnySignal::AnySignal(const char* unit_name, const char* signal_type, const char* signal_target)
:AnyUnit(unit_name)
{
	m_pipeline_time=0;
	m_link_index=-1;
	m_link_node=NULL;

	m_signal_type = signal_type;
	m_signal_target = signal_target;
	this->m_prepared_ok = false;
	this->m_data_update = false;
	this->m_out_degree = 0;

	this->m_signal_type = "UNDEFINED";
	this->m_signal_target = "UNDEFINED";

	m_signal_type_value = EAGLEEYE_UNDEFINED_SIGNAL;
	m_signal_target_value = EAGLEEYE_UNDEFINED_TARGET;

	this->m_signal_exit = false;
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
		else if(!m_link_node->getIsSatisfiedCondition()){
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

	this->m_data_update = false;
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

bool AnySignal::isHasBeenUpdate(){
	return this->m_data_update;
}

void AnySignal::signalHasBeenUpdate()
{
	modified();
	this->m_data_update = true;
}

void AnySignal::getPipelineMonitors(std::map<std::string,std::vector<AnyMonitor*>>& pipeline_monitor_pool)
{
	if (m_link_node){
		m_link_node->getPipelineMonitors(pipeline_monitor_pool);
	}
}

void AnySignal::printUnit(){
	if (m_link_node && m_link_node->getPrintTime() < m_link_node->getPipelinePrintTime()){
		m_link_node->print();
		EAGLEEYE_LOGD("signal: %s (%s) from node: %s (%s)\n",getClassIdentity(),getUnitName(), m_link_node->getUnitName(), m_link_node->getClassIdentity());
	}
}

void AnySignal::wait(){
	if(m_link_node){
		this->m_link_node->wait();
	}
}

void AnySignal::reset(){
	if(m_link_node && m_link_node->getResetTime() < m_link_node->getPipelineResetTime()){
		this->m_link_node->reset();
	}
}

void AnySignal::exit(){
	m_signal_exit = true;
	if(m_link_node && this->m_link_node->getExitTime() < this->m_link_node->getPipelineExitTime()){
		this->m_link_node->exit();
	}	
}

void AnySignal::init(){
	if(m_link_node && m_link_node->getInitTime() < m_link_node->getPipelineInitTime()){
		this->m_link_node->init();
	}	
}

void AnySignal::setSignalType(SignalType type){
	if(type == EAGLEEYE_UNDEFINED_SIGNAL){
		this->m_signal_type = "UNDEFINED";
		EAGLEEYE_LOGE("undefined signal type");
	}
	else{
		this->m_signal_type = SignalTypeName[(int)(type)];
	}

	m_signal_type_value = type;
}

const char* AnySignal::getSignalTypeName(){
	return this->m_signal_type.c_str();
}

void AnySignal::setSignalTarget(SignalTarget target){
	if(target == EAGLEEYE_UNDEFINED_TARGET){
		this->m_signal_target = "UNDEFINED";
		EAGLEEYE_LOGE("undefined signal target");
	}
	else{
		this->m_signal_target = SignalTargetName[(int)(target)];
	}
}

const char* AnySignal::getSignalTarget(){
	return this->m_signal_target.c_str();
}

// void AnySignal::feadback(std::map<std::string, int>& node_state_map){
// 	if (m_link_node){
// 		m_link_node->feadback(node_state_map);
// 	}
// }

bool AnySignal::finish(){
	if (m_link_node){
		bool _is_finish = m_link_node->finish();
		return _is_finish;
	}
	return true;
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

void AnySignal::findIn(AnySignal* ptr, std::vector<std::pair<AnyNode*,int>>& ll){
	if(m_link_node){
		m_link_node->findIn(ptr, ll);
	}
}

void AnySignal::setNeededMem(int size){
	void* mem = malloc(size);
	this->m_mem = std::shared_ptr<unsigned char>((unsigned char*)mem, [](unsigned char* arr) { free(arr); });
}

void* AnySignal::getNeededMem(){
	return this->m_mem.get();
}
}
