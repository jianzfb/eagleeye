#ifndef _EAGLEEYE_TENSORSIGNAL_H_
#define _EAGLEEYE_TENSORSIGNAL_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnySignal.h"
#include "eagleeye/basic/Tensor.h"
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace eagleeye{
class TensorSignal:public AnySignal{
public:
	/**
	 *	@brief define some basic type
	 *	@note you must do these
	 */
    typedef TensorSignal                     	Self;
    typedef AnySignal                       	Superclass;
	typedef Tensor						    	DataType;

    TensorSignal();
    virtual ~TensorSignal();

	/**
	 *	@brief copy info
	 */
	virtual void copyInfo(AnySignal* sig);

	/**
	 *	@brief print TensorSignal info
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
		return new TensorSignal();
	}

	/**
	 * @brief Get the Signal Value Type object
	 * 
	 * @return int 
	 */
	virtual EagleeyeType getValueType(){return m_data.type();};

	/**
	 * @brief Get the Derive Type object
	 * 
	 * @return SignalCategory 
	 */
	virtual SignalCategory getSignalCategory(){return m_sig_category;}

	/**
	 * @brief Get the Signal Content object
	 * 
	 * @param data 
	 * @param data_size 
	 * @param data_dims 
	 */
	virtual void getSignalContent(void*& data, size_t*& data_size, int& data_dims, int& data_type);

	/**
	 * 	@brief 转换成队列
	 */
	virtual void transformCategoryToQ(int max_queue_size=5, bool get_then_auto_remove=true){
		m_sig_category = SIGNAL_CATEGORY_TENSOR_QUEUE;
		m_max_queue_size = max_queue_size;
		this->m_get_then_auto_remove = get_then_auto_remove;
	};

private:
    Tensor m_data;
	Tensor m_tmp;
	SignalCategory m_sig_category;
	int m_release_count;
	bool m_get_then_auto_remove;

	std::queue<Tensor> m_queue;
	int m_max_queue_size;
	std::mutex m_mu;
	std::condition_variable m_cond;	
};
}
#endif