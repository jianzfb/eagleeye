#ifndef _EAGLEEYE_YUVSIGNAL_H_
#define _EAGLEEYE_YUVSIGNAL_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnySignal.h"
#include "eagleeye/basic/type.h"
#include "eagleeye/common/EagleeyeLog.h"
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "eagleeye/basic/blob.h"


namespace eagleeye
{
class YUVSignal:public AnySignal{
public:
    typedef YUVSignal       	Self;
    typedef AnySignal       	Superclass;
	typedef Blob				DataType;


    YUVSignal();
    virtual ~YUVSignal();

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
		return new YUVSignal();
	}

    /**
	 *	@brief clear image signal content
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
	 * @brief Get the Data object
	 * 
	 * @return DataType 
	 */
	Blob getData();

	/**
	 * @brief Set the Data object
	 * 
	 * @param data 
	 */
	void setData(Blob data);

	/**
	 * @brief Get the Data object with meta
	 * 
	 * @param meta 
	 * @return DataType 
	 */
	Blob getData(MetaData& m);

	/**
	 * @brief Set the Data object with meta
	 * 
	 * @param data 
	 * @param meta 
	 */
	void setData(Blob data, MetaData m);

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
	virtual void getSignalContent(void*& data, int* data_size, int& data_dims, int& data_type);

	/**
	 * @brief Get the Signal Value Type object
	 * 
	 * @return int 
	 */
	virtual EagleeyeType getValueType(){return m_yuv_format;};

	/**
	 * @brief Get the Derive Type object
	 * 
	 * @return SignalCategory 
	 */
	virtual SignalCategory getSignalCategory(){return SIGNAL_CATEGORY_IMAGE;}

	/**
	 * @brief Set the Meta object
	 * 
	 * @param meta 
	 */
	virtual void setMeta(MetaData meta);

    /**
     * @brief get width
     */ 
    int getWidth();

    /**
     * @brief get height
     */ 
    int getHeight();

	

    /**
     * @brief print yuv info
     */ 
    void printUnit();

	/**
	 * @brief set val type
	 */ 
	void setValueType(EagleeyeType yuv_format);

private:
    Blob m_blob;
    int m_width;
    int m_height;
    int m_release_count;
	EagleeyeType m_yuv_format;
};
} // namespace eagleeye


#endif