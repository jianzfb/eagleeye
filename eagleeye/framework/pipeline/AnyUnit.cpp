#include "eagleeye/framework/pipeline/AnyUnit.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/render/RenderContext.h"
#include "eagleeye/framework/pipeline/AnyPipeline.h"

namespace eagleeye
{
AnyUnit::AnyUnit(const char* unit_name)
{
	m_unit_name = unit_name;
	m_pipeline_name = "eagleeye";
	modified();
}

AnyUnit::~AnyUnit()
{
	std::vector<AnyMonitor*>::iterator iter,iend(m_unit_monitor_pool.end());
	for (iter = m_unit_monitor_pool.begin(); iter != iend; ++iter)
	{
		if(AnyPipeline::getRenderContext() != NULL){
			AnyPipeline::getRenderContext()->cancelListeningMouse(*iter);
		}
		delete (*iter);
	}
}

AnyMonitor* AnyUnit::getMonitor(int index)
{
	if (index < int(m_unit_monitor_pool.size()))
	{
		return m_unit_monitor_pool[index];
	}
	
	return NULL;
}

int AnyUnit::getMonitorPoolSize()
{
	return m_unit_monitor_pool.size();
}

unsigned long AnyUnit::getMTime() const
{
	return m_mtime.getMTime();
}
void AnyUnit::modified() const
{
	m_mtime.modified();
}
void AnyUnit::frozenMTime() const {
	m_mtime.frozen();
}

void AnyUnit::setPipelineName(const char* pipeline_name)
{
	m_pipeline_name = pipeline_name;
}

void AnyUnit::setUnitPara(std::shared_ptr<char> param_block){
	char* data = (char*)(param_block.get());
	int total_size = *((int*)data);
	data += sizeof(int);

	// 0.step unit name and its memory
	int unit_name_size = *((int*)data);
	data += sizeof(int);
	char* unit_name = data;
	data += unit_name_size;
	EAGLEEYE_LOGD("parse node %s parameters (size %d)", unit_name, total_size);

	// 1.step monitor number
	int monitor_num = *((int*)data);
	data = data + sizeof(int);
	// assert(monitor_num == this->m_unit_monitor_pool.size());
	if(monitor_num != this->m_unit_monitor_pool.size()){
		EAGLEEYE_LOGD("monitor num %d(!=%d) not match in config", monitor_num, this->m_unit_monitor_pool.size());
	}

	// 2.step extract monitor parameter
	for(int i=0; i<monitor_num; ++i){
		// 2.1.step monitor name size
		int name_size = *((int*)data);
		data = data + sizeof(int);

		// 2.2.step monitor name str
		char* name_str = data;
		data = data + name_size;

		// 2.3.step monitor value size
		int value_size = *((int*)data);
		data = data + sizeof(int);

		// 2.4.step monitor value ptr
		char* value_ptr = data;
		data = data + value_size;

		// 2.5.step set monitor value
		for(int index=0; index<this->m_unit_monitor_pool.size(); ++index){
			if(strcmp(this->m_unit_monitor_pool[index]->monitor_var_text, name_str) == 0){
				if(this->m_unit_monitor_pool[index]->monitor_var_type != EAGLEEYE_MONITOR_STR){
					this->m_unit_monitor_pool[index]->setVar(value_ptr);
				}
				else{
					std::string value_str(value_ptr);
					this->m_unit_monitor_pool[index]->setVar(&value_str);
				}
				break;
			}
		}
	}

	int diff = data - param_block.get();
	if(diff != total_size){
		EAGLEEYE_LOGD("node %s config size not match", this->m_unit_name.c_str());
	}
}

void AnyUnit::getUnitPara(std::shared_ptr<char>& param_block){
	// 1.step analyze all monitor size
	// total size
	int total_size = sizeof(int);

	// unit name size
	total_size += sizeof(int);
	// unit name
	total_size += this->m_unit_name.size() + 1;

	// monitor number
	total_size += sizeof(int);

	// unit memory
	for(int index=0; index<this->m_unit_monitor_pool.size(); ++index){
		int value_size = this->m_unit_monitor_pool[index]->monitor_var_size;

		if(value_size == -1 && this->m_unit_monitor_pool[index]->monitor_var_type == EAGLEEYE_MONITOR_STR){
			std::string value_str;
			this->m_unit_monitor_pool[index]->getVar(&value_str);	

			// 1.1.step name size
			total_size += sizeof(int);
			// 1.2.step name str
			total_size += strlen(this->m_unit_monitor_pool[index]->monitor_var_text) + 1;
			// 1.3.step value size
			total_size += sizeof(int);
			// 1.4.step value ptr
			total_size += value_str.size() + 1;
		}
		else if(value_size > 0){
			// 1.1.step name size
			total_size += sizeof(int);
			// 1.2.step name str
			total_size += strlen(this->m_unit_monitor_pool[index]->monitor_var_text) + 1;
			// 1.3.step value size
			total_size += sizeof(int);
			// 1.4.step value ptr
			total_size += value_size;
		}
	}

	// 2.step write to memory
	param_block = std::shared_ptr<char>((char*)malloc(total_size), [](char* data){free(data);});
	char* data = param_block.get();
	memcpy(data, &total_size, sizeof(int));
	data += sizeof(int);

	int unit_name_size = this->m_unit_name.size() + 1;
	memcpy(data, &unit_name_size, sizeof(int));
	data += sizeof(int);

	memcpy(data, this->m_unit_name.c_str(), this->m_unit_name.size());
	data += this->m_unit_name.size();
	data[0] = '\0';
	data += 1;

	int monitor_num = this->m_unit_monitor_pool.size();
	memcpy(data, &monitor_num, sizeof(int));
	data += sizeof(int);

	for(int index=0; index<this->m_unit_monitor_pool.size(); ++index){
		int value_size = this->m_unit_monitor_pool[index]->monitor_var_size;
		if(value_size == -1 && this->m_unit_monitor_pool[index]->monitor_var_type == EAGLEEYE_MONITOR_STR){
			std::string value_str;
			this->m_unit_monitor_pool[index]->getVar(&value_str);	

			// 1.step name size
			int name_size = strlen(this->m_unit_monitor_pool[index]->monitor_var_text) + 1;
			memcpy(data, &name_size, sizeof(int));
			data += sizeof(int);

			// 2.step name str
			memcpy(data, this->m_unit_monitor_pool[index]->monitor_var_text, strlen(this->m_unit_monitor_pool[index]->monitor_var_text));
			data += strlen(this->m_unit_monitor_pool[index]->monitor_var_text);
			data[0] = '\0';
			data += 1;

			// 3.step value size
			value_size = value_str.size() + 1;
			memcpy(data, &value_size, sizeof(int));
			data += sizeof(int);

			// 4.step value ptr
			memcpy(data, value_str.c_str(), value_str.size());
			data += value_str.size();
			data[0] = '\0';
			data += 1;
		}
		else if(value_size > 0 && this->m_unit_monitor_pool[index]->monitor_var_type != EAGLEEYE_MONITOR_UNDEFINED){
			// 1.step name size
			int name_size = strlen(this->m_unit_monitor_pool[index]->monitor_var_text) + 1;
			memcpy(data, &name_size, sizeof(int));
			data += sizeof(int);

			// 2.step name str
			memcpy(data, this->m_unit_monitor_pool[index]->monitor_var_text, strlen(this->m_unit_monitor_pool[index]->monitor_var_text));
			data += strlen(this->m_unit_monitor_pool[index]->monitor_var_text);
			data[0] = '\0';
			data += 1;

			// 3.step value size
			memcpy(data, &value_size, sizeof(int));
			data += sizeof(int);

			// 4.step value ptr
			if(this->m_unit_monitor_pool[index]->monitor_var_type == EAGLEEYE_MONITOR_BOOL){
				bool val;
				this->m_unit_monitor_pool[index]->getVar(&val);
				memcpy(data, &val, value_size);
			}
			else if(this->m_unit_monitor_pool[index]->monitor_var_type == EAGLEEYE_MONITOR_INT){
				int val;
				this->m_unit_monitor_pool[index]->getVar(&val);
				memcpy(data, &val, value_size);
			}
			else if(this->m_unit_monitor_pool[index]->monitor_var_type == EAGLEEYE_MONITOR_FLOAT){
				float val;
				this->m_unit_monitor_pool[index]->getVar(&val);
				memcpy(data, &val, value_size);
			}

			data += value_size;
		}
	}	
}
}
