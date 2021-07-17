#ifndef _GROUP_SIGNAL_H_
#define _GROUP_SIGNAL_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnySignal.h"
#include <vector>

namespace eagleeye
{
class GroupSignal: public AnySignal{
public:
    typedef GroupSignal         Self;
    typedef AnySignal           Superclass;

    /**
	 * @brief data type hold by Image Signal
	 * 
	 */
	typedef std::vector<AnySignal*>				DataType;

    GroupSignal();
    virtual ~GroupSignal();

    /**
	 * @brief make same type signal
	 * 
	 * @return AnySignal* 
	 */
	virtual AnySignal* make(){
		return new GroupSignal();
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
	virtual void copy(AnySignal* sig);

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
	 * @brief Get the Signal Value Type object
	 * 
	 * @return int 
	 */
	virtual EagleeyeType getValueType(){return EAGLEEYE_UNDEFINED;};

private:
    std::vector<AnySignal*> m_data;
};
} // namespace eagleeye


#endif