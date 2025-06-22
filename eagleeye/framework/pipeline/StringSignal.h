#ifndef _EAGLEEYE_STRINGSIGNAL_H_
#define _EAGLEEYE_STRINGSIGNAL_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnySignal.h"
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace eagleeye{
class AnyNode;
class StringSignal:public AnySignal{
public:
    typedef StringSignal                Self;
    typedef AnySignal                   Superclass;

	typedef std::string					DataType;

	StringSignal(std::string ini_str="");
	virtual ~StringSignal();

	/**
	 * @brief Set the Init object
	 * 
	 * @param ini_str 
	 */
	void setInit(std::string ini_str);

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
		return new StringSignal();
	}

	/**
	 *	@brief print StringSignal info
	 */
	virtual void printUnit();

	/**
	 * @brief Get the Data object
	 * 
	 * @return DataType 
	 */
	DataType getData();

	/**
	 * @brief Get the Data object with meta
	 * 
	 * @param meta 
	 * @return DataType 
	 */
	DataType getData(MetaData& mm);

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
	virtual void transformCategoryToQ(int max_queue_size=5, bool get_then_auto_remove=true, bool set_then_auto_remove=true){
		m_sig_category = SIGNAL_CATEGORY_STRING_QUEUE;
		m_max_queue_size = max_queue_size;
		this->m_get_then_auto_remove = get_then_auto_remove;
		this->m_set_then_auto_remove = set_then_auto_remove;
	};

	/**
	 * @brief 唤醒等待状态（对于队列模式，在某些情况下，需要唤醒帮助处理临时逻辑）
	 */
	virtual void wake();

	virtual bool tryClear();

private:
    std::string m_str;
	std::string m_tmp_cache;
	std::queue<std::pair<std::string, int>> m_queue;
	std::queue<std::pair<MetaData, int>> m_meta_queue;
	SignalCategory m_sig_category;

	std::mutex m_mu;
	std::condition_variable m_cond;

	size_t m_data_size[1];
	std::string m_ini_str;
	int m_max_queue_size;

	bool m_get_then_auto_remove;
	bool m_set_then_auto_remove;
};


class ListStringSignal:public AnySignal{
public:
    typedef ListStringSignal          	Self;
    typedef AnySignal                   Superclass;

	typedef std::vector<std::string>	DataType;

	ListStringSignal();
	virtual ~ListStringSignal();

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
		return new ListStringSignal();
	}

	/**
	 *	@brief print ListStringSignal info
	 */
	virtual void printUnit();

	/**
	 * @brief Get the Data object
	 * 
	 * @return DataType 
	 */
	DataType getData();

	/**
	 * @brief Get the Data object with meta
	 * 
	 * @param meta 
	 * @return DataType 
	 */
	DataType getData(MetaData& mm);

	/**
	 * @brief Set the Data object
	 * 
	 * @param data 
	 */
	void setData(DataType data);
	void setData(DataType data, MetaData mm);

	/**
	 *	@brief clear Info signal content
	 */
	virtual void makeempty(bool auto_empty=true);

	virtual bool tryClear();

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
	 * @brief to SIGNAL_CATEGORY_IMAGE_QUEUE
	 * 
	 */
	virtual void transformCategoryToQ(int max_queue_size=5, bool get_then_auto_remove=true, bool set_then_auto_remove=true){
		m_sig_category = SIGNAL_CATEGORY_LIST_STRING_QUEUE;
		m_max_queue_size = max_queue_size;

		m_get_then_auto_remove = get_then_auto_remove;
		m_set_then_auto_remove = set_then_auto_remove;
	};

	/**
	 * @brief 唤醒等待状态（对于队列模式，在某些情况下，需要唤醒帮助处理临时逻辑）
	 */
	virtual void wake();

private:
	std::vector<std::string> m_list;
	std::queue<std::pair<std::vector<std::string>, int>> m_queue;
	std::queue<std::pair<MetaData, int>> m_meta_queue;
	SignalCategory m_sig_category;

	std::mutex m_mu;
	std::condition_variable m_cond;
	int m_max_queue_size;

	bool m_get_then_auto_remove;
	bool m_set_then_auto_remove;
};

}
#endif