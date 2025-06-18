#ifndef _EAGLEEYE_BOOLEANSIGNAL_H_
#define _EAGLEEYE_BOOLEANSIGNAL_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnySignal.h"

namespace eagleeye{

class BooleanSignal:public AnySignal
{
public:
	/**
	 *	@brief define some basic type
	 *	@note you must do these
	 */
	typedef BooleanSignal							Self;
	typedef AnySignal								Superclass;

	typedef bool									MetaType;
	typedef bool									DataType;

	BooleanSignal(bool ini_boolean=false);
	virtual ~BooleanSignal();

	/**
	 * @brief Set the Init object
	 * 
	 * @param ini_boolean 
	 */
	void setInit(bool ini_boolean);

	/**
	 *	@brief copy info
	 */
	virtual void copyInfo(AnySignal* sig);

	/**
	 *	@brief print BooleanSignal info
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
	 * @brief Set the Signal Content object
	 * 
	 * @param data 
	 * @param data_size 
	 * @param data_dims 
	 */
	virtual void setData(void* data, MetaData meta);

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
	virtual void copy(AnySignal* sig, bool is_deep=false);

	/**
	 * @brief make same type signal
	 * 
	 * @return AnySignal* 
	 */
	virtual AnySignal* make(){
		return new BooleanSignal();
	}

	/**
	 * @brief Get the Signal Value Type object
	 * 
	 * @return int 
	 */
	virtual EagleeyeType getValueType(){return EAGLEEYE_BOOL;};

	/**
	 * @brief Get the Derive Type object
	 * 
	 * @return SignalCategory 
	 */
	virtual SignalCategory getSignalCategory(){return SIGNAL_CATEGORY_CONTROL;}

private:
	bool m_boolean;	
	bool m_ini_boolean;
};

}
#endif