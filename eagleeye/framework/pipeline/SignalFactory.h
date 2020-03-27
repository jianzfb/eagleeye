#ifndef _SIGNALFACTORY_H_
#define _SIGNALFACTORY_H_

#include "eagleeye/common/EagleeyeMacro.h"

#include "eagleeye/framework/pipeline/AnySignal.h"
#include "eagleeye/framework/pipeline/BooleanSignal.h"
#include "eagleeye/framework/pipeline/StringSignal.h"
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
struct ImageMetaData{
	std::string name;		// name
	std::string info;		// info
	double fps;				// frame rate for video
	int nb_frames;			// frame number for video
	int frame;				// frame index for video
	bool is_end_frame;		// end frame flag for video
	bool is_start_frame; 	// start frame flag for video
	int rotation;			// rotation  (0,90,180,270)
	int needed_rows;		// rows
	int needed_cols;		// cols
};
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
			this->m_release_count = 1;
		}
	virtual ~BaseImageSignal(){};

	/**
	 * @brief get image meta information
	 * 
	 * @return ImageMetaData* 
	 */
	ImageMetaData* meta(){
		return &m_meta;
	}

	const EagleeyeType pixel_type;
	const int channels;

protected:
	int m_release_count;
	ImageMetaData m_meta;
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
		return new ImageSignal<T>();
	}

	/**
	 *	@brief print image signal info
	 */
	virtual void printUnit();

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

	// /**
	//  *	@brief create one image
	//  *	@note most of time, we should use this function to load one image,\n
	//  *	it would force updating 'data time'
	//  */
	// virtual void createImage(Matrix<T> m,char* name="",char* info="");

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
	 * @brief Get the Signal Value Type object
	 * 
	 * @return int 
	 */
	EagleeyeType getSignalValueType(){return TypeTrait<T>::type;};

	/**
	 * @brief Get the Derive Type object
	 * 
	 * @return SignalCategory 
	 */
	virtual SignalCategory getSignalCategoryType(){return SIGNAL_CATEGORY_IMAGE;}

private:
	Matrix<T> img;
};

/* Tensor Signal */
template<class T>
class TensorSignal:public AnySignal{
public:
	/**
	 *	@brief define some basic type
	 *	@note you must do these
	 */
	typedef TensorSignal				Self;
	typedef AnySignal					Superclass;

	/**
	 * @brief data type hold by Image Signal
	 * 
	 */
	typedef Tensor<T>				DataType;

	/**
	 * @brief Construct a new Image Signal object
	 * 
	 * @param m data
	 * @param name data name
	 * @param info related infomation
	 */
	TensorSignal(Tensor<T> m=Tensor<T>(),char* n="",char* info="");
	virtual ~TensorSignal();

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
		return new TensorSignal<T>();
	}

	/**
	 *	@brief print image signal info
	 */
	virtual void printUnit();

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
	DataType getData();

	/**
	 * @brief Set the Data object
	 * 
	 * @param data 
	 */
	void setData(DataType data);

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
	 * @brief Get the Signal Value Type object
	 * 
	 * @return int 
	 */
	EagleeyeType getSignalValueType(){return TypeTrait<T>::type;};

	/**
	 * @brief Get the Derive Type object
	 * 
	 * @return SignalCategory 
	 */
	virtual SignalCategory getSignalCategoryType(){return SIGNAL_CATEGORY_TENSOR;}


private:
	Tensor<T> m_data;
	int m_release_count;
};

//////////////////////////////////////////////////////////////////////////
template<class T>
class ContentSignal:public AnySignal
{
public:
	/**
	 *	@brief define some basic type
	 *	@note you must do these
	 */
	typedef ContentSignal						Self;
	typedef AnySignal							Superclass;

	typedef T									MetaType;
	typedef T									DataType;

	ContentSignal(T sth_info=T()):info(sth_info){};
	virtual ~ContentSignal(){};

	/**
	 *	@brief copy info
	 */
	virtual void copyInfo(AnySignal* sig);

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
	 * @brief Get the Signal Content object
	 * 
	 * @param data 
	 * @param data_size 
	 * @param data_dims 
	 * @param data_type 
	 */
	virtual void getSignalContent(void*& data, int* data_size, int& data_dims, int& data_type);

	/**
	 * @brief Set the Signal Content object
	 * 
	 * @param data 
	 * @param data_size 
	 * @param data_dims 
	 */
	virtual void setSignalContent(void* data, const int* data_size, const int data_dims);
	T info;
};
}

#include "SignalFactory.hpp"
#endif
