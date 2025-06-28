#ifndef _EAGLEEYE_JSONSIGNAL_H_
#define _EAGLEEYE_JSONSIGNAL_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnySignal.h"
#include "eagleeye/common/CJsonObject.hpp"
#include "eagleeye/common/EagleeyeMessageCenter.h"
#include "eagleeye/common/EagleeyeMessage.h"
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace eagleeye{
class AnyNode;
class JsonSignal:public AnySignal{
public:
    typedef JsonSignal                  Self;
    typedef AnySignal                   Superclass;

	typedef std::string					DataType;

	JsonSignal(std::string record_name="", bool is_record_in_message_center=false);
	virtual ~JsonSignal();

	/**
	 *	@brief copy info
	 */
	virtual void copyInfo(AnySignal* sig);

	/**
	 * @brief copy content
	 * 
	 * @param sig 
	 */
	virtual void copy(AnySignal* sig, bool is_deep=false);

	/**
	 * @brief make same type signal
	 * 
	 * @return AnySignal* 
	 */
	virtual AnySignal* make(){
		return new JsonSignal();
	}

	/**
	 *	@brief print JsonSignal info
	 */
	virtual void printUnit();

	/**
	 * @brief Get the Data object
	 * 
	 * @return DataType 
	 */
	DataType getData(bool deep_copy=false);

	/**
	 * @brief Get the Data object with meta
	 * 
	 * @param meta 
	 * @return DataType 
	 */
	DataType getData(MetaData& mm, bool deep_copy=false);

	DataType getDataWithId(std::string id, MetaData& mm, bool deep_copy=false);

	/**
	 * @brief Set the Data object
	 * 
	 * @param data 
	 */
	void setData(DataType data);

	/**
	 * @brief Set the Data object with meta
	 * 
	 * @param data 
	 * @param meta 
	 */
	void setData(DataType data, MetaData mm);

	/**
	 * @brief Set the Data object
	 * 
	 * @param data 
	 */
    void setKV(std::string key, std::string value);

	/**
	 * @brief Set the Data object
	 * 
	 * @param data 
	 */
    void setKList(std::string key, std::vector<int> value);
    void setKList(std::string key, std::vector<float> value);
    void setKList(std::string key, std::vector<double> value);
    void setKList(std::string key, std::vector<std::string> value);
	void setKT(std::string key, std::vector<float> value, EagleeyeType type, std::vector<int> dims);

	/**
	 *	@brief clear Info signal content
	 */
	virtual void makeempty(bool auto_empty=true);

	/**
	 * @brief check signal content empty
	 * 
	 * @return true 
	 * @return false 
	 */
	virtual bool isempty();

	/**
	 * @brief Get the Signal Value Type object
	 * 
	 * @return int 
	 */
	virtual EagleeyeType getValueType(){return EAGLEEYE_STRING;};

	/**
	 * @brief Get the Derive Type object
	 * 
	 * @return SignalCategory 
	 */
	virtual SignalCategory getSignalCategory(){return m_sig_category;}

	/**
	 * @brief Set the Signal Content object
	 * 
	 * @param data 
	 * @param data_size 
	 * @param data_dims 
	 */
	virtual void setData(void* data, MetaData meta);

	/**
	 *	@brief flush to message center
	 */
	void flush();

	/**
	 * @brief Get the Signal Content object
	 * 
	 * @param data 
	 * @param data_size 
	 * @param data_dims 
	 */
	virtual void getSignalContent(void*& data, size_t*& data_size, int& data_dims, int& data_type);

	/**
	 * @brief to SIGNAL_CATEGORY_IMAGE_QUEUE
	 * 
	 */
	virtual void transformCategoryToQ(int max_queue_size=5, bool get_then_auto_remove=true, bool set_then_auto_remove=true){
		m_sig_category = SIGNAL_CATEGORY_STRING_QUEUE;
		m_max_queue_size = max_queue_size;
		this->m_get_then_auto_remove = get_then_auto_remove;
		this->m_set_then_auto_remove = set_then_auto_remove;
	};

	virtual bool tryClear();

private:
	std::string m_tmp_cache;
	std::queue<std::pair<std::string, int>> m_queue;
	std::queue<std::pair<MetaData, int>> m_meta_queue;
	std::string m_info;
	SignalCategory m_sig_category;

	std::mutex m_mu;
	std::condition_variable m_cond;

	size_t m_data_size[1];
	int m_max_queue_size;

    neb::CJsonObject m_json_obj;
	std::string m_record_name;
	bool m_is_record_in_message_center;
	bool m_get_then_auto_remove;
	bool m_set_then_auto_remove;
};

}
#endif