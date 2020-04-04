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

	StringSignal();
	virtual ~StringSignal();

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
		return new StringSignal();
	}

	/**
	 *	@brief print ContentSignal info
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
	EagleeyeType getSignalValueType(){return EAGLEEYE_STRING;};

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
	virtual void setSignalContent(void* data, const int* data_size, const int data_dims);

	/**
	 * @brief Get the Signal Content object
	 * 
	 * @param data 
	 * @param data_size 
	 * @param data_dims 
	 */
	virtual void getSignalContent(void*& data, int* data_size, int& data_dims, int& data_type);

	/**
	 * @brief to SIGNAL_CATEGORY_IMAGE_QUEUE
	 * 
	 */
	virtual void transformCategoryToQ(){m_sig_category = SIGNAL_CATEGORY_STRING_QUEUE;};


private:
    std::string m_str;
	std::string m_tmp;
	std::queue<std::string> m_queue;	
	SignalCategory m_sig_category;

	std::mutex m_mu;
	std::condition_variable m_cond;
};

}
#endif