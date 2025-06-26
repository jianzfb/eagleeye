#ifndef _LANDMARK_SIGNAL_H_
#define _LANDMARK_SIGNAL_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnySignal.h"
#include "eagleeye/basic/Matrix.h"

namespace eagleeye
{
class LandmarkSignal:public AnySignal{
public:
    typedef LandmarkSignal      Self;
    typedef AnySignal           Superclass;

    /**
	 * @brief data type hold by Image Signal
	 * 
	 */
	typedef Matrix<float>				DataType;

    LandmarkSignal(Matrix<float> landmark=Matrix<float>());
    virtual ~LandmarkSignal();

    /**
	 * @brief make same type signal
	 * 
	 * @return AnySignal* 
	 */
	virtual AnySignal* make(){
		return new LandmarkSignal();
	}

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
	DataType getData(bool deep_copy=false);
    
	/**
	 * @brief Get the Data object with meta
	 * 
	 * @param meta 
	 * @return DataType 
	 */
	DataType getData(MetaData& mm, bool deep_copy=false);

	/**
	 * @brief Set the Data object
	 * 
	 * @param data 
	 */
	void setData(DataType data);

	/**
	 * @brief Set the Meta object
	 * 
	 * @param meta 
	 */
	virtual void setMeta(MetaData meta);

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
	 * @brief Get the Signal Value Type object
	 * 
	 * @return int 
	 */
	virtual EagleeyeType getValueType(){return TypeTrait<float>::type;};

    /**
     * @brief Set the Joints object
     * 
     * @param joints 
     */
    void setJoints(Matrix<int> joints);

    /**
     * @brief Get the Joints object
     * 
     * @return Matrix<int> 
     */
    Matrix<int> getJoints();


private:
    Matrix<float> m_landmark;
    Matrix<int> m_joints;
};    
} // namespace eagleeye


#endif