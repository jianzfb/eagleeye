#ifndef _EAGLEEYE_JSONSIGNAL_H_
#define _EAGLEEYE_JSONSIGNAL_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnySignal.h"
#include "eagleeye/common/CJsonObject.hpp"
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

	JsonSignal();
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
	virtual void copy(AnySignal* sig);

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
	DataType getData();

	/**
	 * @brief Set the Data object
	 * 
	 * @param data 
	 */
	void setData(DataType data);

	/**
	 * @brief Set the Data object
	 * 
	 * @param data 
	 */
    void setKV(std::string key, std::string value);

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
	virtual void transformCategoryToQ(int max_queue_size=5, bool get_then_auto_remove=true){
		m_sig_category = SIGNAL_CATEGORY_STRING_QUEUE;
		m_max_queue_size = max_queue_size;
	};

private:
	std::string m_tmp_cache;
	std::queue<std::string> m_queue;
	std::string m_info;
	SignalCategory m_sig_category;

	std::mutex m_mu;
	std::condition_variable m_cond;

	size_t m_data_size[1];
	int m_release_count;
	int m_max_queue_size;

    neb::CJsonObject m_json_obj;
};

}
#endif