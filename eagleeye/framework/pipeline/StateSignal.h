#ifndef _EAGLEEYE_STATESIGNAL_H_
#define _EAGLEEYE_STATESIGNAL_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnySignal.h"

namespace eagleeye{
class StateSignal:public AnySignal{
public:
	/**
	 *	@brief define some basic type
	 *	@note you must do these
	 */
    typedef StateSignal                     Self;
    typedef AnySignal                       Superclass;

	typedef int								MetaType;
	typedef int								DataType;

    StateSignal(int ini_state=0);
    virtual ~StateSignal();

	/**
	 * @brief Set the Init object
	 * 
	 * @param ini_state 
	 */
	void setInit(int ini_state);

	/**
	 *	@brief copy info
	 */
	virtual void copyInfo(AnySignal* sig);

	/**
	 *	@brief print StateSignal info
	 */
	virtual void printUnit();

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
		return new StateSignal();
	}

	/**
	 * @brief Get the Signal Value Type object
	 * 
	 * @return int 
	 */
	virtual EagleeyeType getValueType(){return EAGLEEYE_INT;};

	/**
	 * @brief Get the Derive Type object
	 * 
	 * @return SignalCategory 
	 */
	virtual SignalCategory getSignalCategory(){return SIGNAL_CATEGORY_STATE;}

	/**
	 * @brief Get the Signal Content object
	 * 
	 * @param data 
	 * @param data_size 
	 * @param data_dims 
	 */
	virtual void getSignalContent(void*& data, size_t*& data_size, int& data_dims, int& data_type);

private:
	int m_state;	
    int m_ini_state;
	size_t m_data_size[1];
};
}
#endif