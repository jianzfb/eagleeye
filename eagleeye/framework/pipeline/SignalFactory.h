#ifndef _SIGNALFACTORY_H_
#define _SIGNALFACTORY_H_

#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnySignal.h"
#include "eagleeye/framework/pipeline/BooleanSignal.h"
#include "eagleeye/framework/pipeline/StringSignal.h"
#include "eagleeye/framework/pipeline/StateSignal.h"
#include "eagleeye/framework/pipeline/YUVSignal.h"
#include "eagleeye/framework/pipeline/TensorSignal.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/basic/type.h"
#include "eagleeye/common/EagleeyeLog.h"
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace eagleeye
{
//////////////////////////////////////////////////////////////////////////
class EAGLEEYE_API BaseImageSignal:public AnySignal
{
public:
	/**
	 *	@brief define some basic type
	 *	@note you must do these
	 */
	typedef BaseImageSignal						Self;
	typedef AnySignal							Superclass;

	BaseImageSignal(EagleeyeType p_type,int cha,char* info = "")
		:AnySignal("ImgSig"),pixel_type(p_type),
		channels(cha){
			this->setSignalType(EAGLEEYE_SIGNAL_IMAGE);
		}
	virtual ~BaseImageSignal(){};


	const EagleeyeType pixel_type;
	const int channels;
	
protected:
	std::mutex m_mu;
	std::condition_variable m_cond;
};

//////////////////////////////////////////////////////////////////////////
template<class T>
class ImageSignal:public BaseImageSignal
{
public:
	/**
	 *	@brief define some basic type
	 *	@note you must do these
	 */
	typedef ImageSignal				Self;
	typedef BaseImageSignal			Superclass;

	/**
	 *	@brief meta type hold by Image Signal
	 */
	typedef T						MetaType;

	/**
	 * @brief data type hold by Image Signal
	 * 
	 */
	typedef Matrix<T>				DataType;

	/**
	 * @brief Construct a new Image Signal object
	 * 
	 * @param m data
	 * @param name data name
	 * @param info related infomation
	 */
	ImageSignal(Matrix<T> m=Matrix<T>(),char* n="",char* info="");
	virtual ~ImageSignal(){};

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
		ImageSignal<T>* signal_cp = new ImageSignal<T>();
		signal_cp->setSignalType(this->getSignalType());
		return signal_cp;
	}

	/**
	 *	@brief print image signal info
	 */
	virtual void printUnit();

	/**
	 *	@brief clear image signal content
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
	 * @brief get waiting queue size (only for queue mode)
	 */
	virtual int getQueueSize();

	/**
	 * @brief Get the Data object
	 * 
	 * @return DataType 
	 */
	DataType getData(bool deep_copy=false);

	/**
	 * @brief Set the Data object
	 * 
	 * @param data 
	 */
	void setData(DataType data);

	/**
	 * @brief Get the Data object with meta
	 * 
	 * @param meta 
	 * @return DataType 
	 */
	DataType getData(MetaData& mm, bool deep_copy=false);

	/**
	 * @brief Set the Data object with meta
	 * 
	 * @param data 
	 * @param meta 
	 */
	void setData(DataType data, MetaData mm);

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
	virtual void getSignalContent(void*& data, size_t*& data_size, int& data_dims, int& data_type, MetaData& data_meta);

	/**
	 * @brief Get the Signal Value Type object
	 * 
	 * @return int 
	 */
	virtual EagleeyeType getValueType(){return TypeTrait<T>::type;};

	/**
	 * @brief Get the Derive Type object
	 * 
	 * @return SignalCategory 
	 */
	virtual SignalCategory getSignalCategory(){return m_sig_category;}
	
	/**
	 * @brief to SIGNAL_CATEGORY_IMAGE_QUEUE
	 * 
	 */
	virtual void transformCategoryToQ(int max_queue_size=5, bool get_then_auto_remove=true, bool set_then_auto_remove=true){
		m_sig_category = SIGNAL_CATEGORY_IMAGE_QUEUE;
		m_max_queue_size = max_queue_size;
		this->m_get_then_auto_remove = get_then_auto_remove;
		this->m_set_then_auto_remove = set_then_auto_remove;
	};

	/**
	 * @brief Set the Meta object
	 * 
	 * @param meta 
	 */
	virtual void setMeta(MetaData meta);

	/**
	 * @brief 唤醒等待状态（对于队列模式，在某些情况下，需要唤醒帮助处理临时逻辑）
	 */
	virtual void wake();

private:
	Matrix<T> img;
	Matrix<T> m_tmp;
	size_t m_data_size[3];
	MetaData m_tmp_meta;
	std::queue<std::pair<Matrix<T>, int>> m_queue;
	std::queue<std::pair<MetaData, int>> m_meta_queue;
	SignalCategory m_sig_category;
	int m_max_queue_size;
	bool m_get_then_auto_remove;
	bool m_set_then_auto_remove;

	int record_count;
};
}

#include "SignalFactory.hpp"
#endif
