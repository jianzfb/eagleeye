#ifndef _ANYSIGNAL_H_
#define _ANYSIGNAL_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyUnit.h"
#include <stdio.h>
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <condition_variable>

namespace eagleeye
{
class MetaData{
public:
	MetaData(){
		name = "";
		info = "";
		fps = 0.0;
		nb_frames = 0;
		frame = 0;
		is_end_frame = false;
		is_start_frame = false;
		is_pause_frame = false;
		is_snapshot_frame = false;
		rotation = 0;
		rows = 0;
		cols = 0;
		needed_rows = 0;
		needed_cols = 0;	
		allocate_mode = 0;
		timestamp = 0.0;
		mirror = false;
		color_format = -1;
		type = -1;
		disable = false;
	}
	std::string name;		// name
	std::string info;		// info
	double fps;				// frame rate for video
	int nb_frames;			// frame number for video
	int frame;				// frame index for video
	bool is_end_frame;		// end frame flag for video
	bool is_start_frame; 	// start frame flag for video
	bool is_pause_frame;	// pause frame flag for video
	bool is_snapshot_frame;	// snapshot
	int rotation;			// rotation  (0,90,180,270)
	int rows;				// current rows 
	int cols;				// current cols
	int needed_rows;		// rows(largest)
	int needed_cols;		// cols(largest)
	int allocate_mode;		// 0（do nothing）;1（InPlace）;2（largest）;3（same size with input）;
	double timestamp;		// timestamp
	bool mirror;			// mirror(mirror, rotation)
	int color_format;		// color space(// -1: UNKOWN, 0: RGB, 1: BGR, 2: RGBA, 3: BGRA, 4: YUV420P, 5: YUV420SP)

	int type;					// 数据类型
	std::vector<int64_t> dims;	// 数据维度

	bool disable;			// 失活标记（标记数据无效）
};

class AnyNode;
class EAGLEEYE_API AnySignal:public AnyUnit
{
public:
	/**
	 *	@brief define some basic type
	 *	@note you must do these
	 */
	typedef AnySignal							Self;
	typedef AnyUnit								Superclass;

	AnySignal(const char* unit_name = "AnySignal", 
				const char* signal_type="", 
				const char* signal_target="");
	virtual ~AnySignal();

	/**
	 *	@brief Get class identity
	 */
	EAGLEEYE_CLASSIDENTITY(AnySignal);

	/**
	 *	@brief Set/Get pipeline time controlled by AnyNode
	 */
	unsigned long getPipelineTime();
	void setPipelineTime(unsigned long time);

	/**
	 *	@brief Link/Dislink AnyNonde 
	 */
	void linkAnyNode(AnyNode* node,int index);
	void dislinkAnyNode(AnyNode* node,int index);

	/**
	 *	@brief Copy info from signal
	 *	@note If you want to copy some specific info, you have to
	 *	overload this function. This function would be called by "passonNodeInfo"
	 *	of AnyNode, implicitly.
	 */
	virtual void copyInfo(AnySignal* sig){
		if(sig == NULL){
			return;
		}

		this->m_prepared_ok = this->m_prepared_ok&sig->isPreparedOK();
		// 传递时间戳
		this->m_meta.timestamp = sig->meta().timestamp;
	};
	
	/**
	 * @brief copy content from signal
	 * 
	 * @param sig 
	 */
	virtual void copy(AnySignal* sig, bool is_deep=false){};

	/**
	 * @brief clone signal
	 * 
	 * @return AnySignal* 
	 */
	virtual AnySignal* make(){return NULL;}

	/**
	 *	@brief Deliver the update flow
	 */
	virtual void updateUnitInfo();

	/**
	 *	@brief According to update time, notify the linked node
	 *	to process unit info dynamically.
	 */
	virtual void processUnitInfo();

	/**
	 *	@brief print this signal unit info
	 *	@note this function would be called by AnyNode object
	 */
	virtual void printUnit();

	/**
	 * @brief wait node
	 */ 
	virtual void wait();

	/**
	 * @brief reset node backward
	 * 
	 */
	virtual void reset();

	/**
	 * @brief exit node backward
	 * 
	 */
	virtual void exit();

	/**
	 * @brief init node
	 * 
	 */
	virtual void init();

	/**
	 *	@brief one easy explained function, which notify itself
	 *	update.
	 */
	void signalHasBeenUpdate();

	/**
	 * @brief check signal content whether has been update
	 */ 
	bool isHasBeenUpdate();

	/**
	 *	@brief get monitor pool of the whole pipeline
	 *	@note traverse the whole pipeline
	 */
	void getPipelineMonitors(std::map<std::string,std::vector<AnyMonitor*>>& pipeline_monitor_pool);

	/**
	 *	@brief clear signal content
	 */
	virtual void makeempty(bool auto_empty=true){};

	/**
	 * @brief check signal content
	 * 
	 * @return true 
	 * @return false 
	 */
	virtual bool isempty(){return false;};

	/**
	 * @brief Set the Signal Type object
	 * 
	 * @param type 
	 */
	void setSignalType(SignalType type);

	/**
	 * @brief Get the Signal Type object
	 * 
	 * @return const char* 
	 */
	const char* getSignalTypeName();
	
	/**
	 * @brief Get the Signal Type Value object
	 * 
	 * @return SignalType 
	 */
	SignalType getSignalType(){return this->m_signal_type_value;};

	/**
	 * @brief Set the Signal Target object
	 * 
	 * @param target 
	 */
	void setSignalTarget(SignalTarget target);

	/**
	 * @brief Get the Signal Target object
	 * 
	 * @return const char* 
	 */
	const char* getSignalTarget();
	
	/**
	 * @brief Get the Signal Target Value object
	 * 
	 * @return SignalTarget 
	 */
	SignalTarget getSignalTargetValue(){return this->m_signal_target_value;};

	/**
	 * @brief Set the Signal Content object
	 * 
	 * @param data 
	 * @param data_size 
	 * @param data_dims 
	 */
	virtual void setData(void* data, MetaData meta){};

	/**
	 * @brief Get the Signal Content object
	 * 
	 * @param data 
	 * @param data_size 
	 * @param data_dims 
	 */
	virtual void getSignalContent(void*& data, size_t*& data_size, int& data_dims, int& data_type){};
	virtual void getSignalContent(void*& data, size_t*& data_size, int& data_dims, int& data_type, MetaData& data_meta){};

	/**
	 * @brief whether content is prepared in the current signal 
	 * 
	 * @return true 
	 * @return false 
	 */
	virtual bool isPreparedOK(){return this->m_prepared_ok;}
	
	/**
	 * @brief increment out degree
	 * 
	 */
	void incrementOutDegree(){
		this->m_out_degree += 1;
	};
	/**
	 * @brief decrement out degree
	 * 
	 */
	void decrementOutDegree(){
		this->m_out_degree -= 1;
	};

	int getOutDegree(){return this->m_out_degree;}

	// /**
	//  * @brief feadback
	//  * 
	//  * @param node_state_map 
	//  */
	// void feadback(std::map<std::string, int>& node_state_map);

	/**
	 * @brief call after start
	 * 
	 * @return true 
	 * @return false 
	 */
	virtual bool finish();

	/**
	 * @brief Get the Derive Type object
	 * 
	 * @return SignalCategory 
	 */
	virtual SignalCategory getSignalCategory(){return EAGLEEYE_UNDEFINED_CATEGORY;}
	
	/**
	 * @brief upgrade signal to queue version
	 * 
	 */
	virtual void transformCategoryToQ(int max_queue_size=5, bool get_then_auto_remove=true, bool set_then_auto_remove=true){};

	/**
	 * @brief Get the Signal Value Type object
	 * 
	 * @return int 
	 */
	virtual EagleeyeType getValueType(){return EAGLEEYE_UNDEFINED;}

	/**
	 * @brief load/save pipeline configure
	 * 
	 * @param node_config 
	 */
	void loadConfigure(std::map<std::string, std::shared_ptr<char>> nodes_config);
	void saveConfigure(std::map<std::string, std::shared_ptr<char>>& nodes_config);

	/**
	 * @brief get metadata
	 * 
	 * @return MetaData
	 */
	virtual MetaData& meta(){return this->m_meta;};

	/**
	 * @brief Set the Meta object
	 * 
	 * @param meta 
	 */
	virtual void setMeta(MetaData meta){this->m_meta = meta;};

	/**
	 * @brief find input signal
	 * 
	 * @param ptr 
	 * @param ll 
	 */
	void findIn(AnySignal* ptr, std::vector<std::pair<AnyNode*,int>>& ll);

	/**
	 * @brief Set the Needed Mem object
	 * 
	 */
	void setNeededMem(int size);

	/**
	 * @brief Get the Needed Mem object
	 * 
	 * @return void* 
	 */
	void* getNeededMem();

	/*
	 * @brief Get link node
	 */
	AnyNode* getLinkNode(){return this->m_link_node;}

	/**
	 * @brief 禁用数据时间戳
	 */
	void disableDataTimestamp(){m_disable_data_timestamp = true;}
	
	/**
	 * @brief 启用无效标记（仅在管线退出时，标记）
	 */
	void disable(){m_disable = true;}

	/**
	 * @brief 启用无效标记（仅在管线退出时，标记）
	 */
	bool isEnable(){return !m_disable;}

	/**
	 * @brief 唤醒等待状态（对于队列模式，在某些情况下，需要唤醒帮助处理临时逻辑）
	 */
	virtual void wake(){};

	/**
	 *	@brief 触发自动清除队列元素（一般用在队列模式下，手动管理队列元素）
	 */
	virtual bool tryClear(){};

protected:
	std::string m_signal_type;
	std::string m_signal_target;

	SignalType m_signal_type_value;
	SignalTarget m_signal_target_value;

	bool m_prepared_ok;
	bool m_data_update;
	AnyNode* m_link_node;
	MetaData m_meta;
	std::shared_ptr<unsigned char> m_mem;
	bool m_disable_data_timestamp;
	bool m_disable;

private:
	AnySignal(const AnySignal&);
	void operator=(const AnySignal&);

	unsigned long m_pipeline_time;
	int m_link_index;
	int m_out_degree;
};
}

#endif
