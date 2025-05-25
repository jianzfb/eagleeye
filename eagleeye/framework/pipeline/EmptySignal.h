#ifndef _EAGLEEYE_EMPTYSIGNAL_H_
#define _EAGLEEYE_EMPTYSIGNAL_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnySignal.h"


namespace eagleeye
{
class EmptySignal:public AnySignal{
public:
	/**
	 *	@brief define some basic type
	 *	@note you must do these
	 */
    typedef EmptySignal                     Self;
    typedef AnySignal                       Superclass;

	typedef bool							MetaType;
	typedef bool							DataType;

    EmptySignal();
    virtual ~EmptySignal();

    /**
	 *	@brief copy info
	 */
	virtual void copyInfo(AnySignal* sig);

    /**
	 * @brief Get the Data object
	 * 
	 * @return DataType 
	 */
	bool getData(){return true;};

    /**
	 * @brief Set the Data object
	 * 
	 * @param data 
	 */
	void setData(bool data){};

    /**
	 *	@brief clear Info signal content
	 */
	virtual void makeempty(bool auto_empty=true){};

    /**
	 * @brief check signal content empty
	 * 
	 * @return true 
	 * @return false 
	 */
	virtual bool isempty(){return false;};

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
		return new EmptySignal();
	}
};  
} // namespace eagleeye

#endif