#ifndef _EAGLEEYE_DATASOURCE_H_
#define _EAGLEEYE_DATASOURCE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/common/EagleeyeIO.h"
#include "eagleeye/processnode/AnyPlaceholder.h"

namespace eagleeye{
template<class T>
class Placeholder:public AnyPlaceholder{
public:
    typedef Placeholder                  	Self;
    typedef AnyNode                         Superclass;

	/**
	 * @brief Construct a new Data Source Node object
	 * 
	 * @param unit_name 
	 * @param type 
	 * @param source 
	 * @param queue_mode 
	 */
    Placeholder(bool queue_mode=false);
    virtual ~Placeholder();

    /**
	 *	@brief get class identity
	 */
	EAGLEEYE_CLASSIDENTITY(Placeholder);

    /**
	 *	@brief define output port image signal type
	 */
	EAGLEEYE_OUTPUT_PORT_TYPE(T,	0,	PLACEHOLDER);

	/**
	 *	@brief 
	 */
	virtual void executeNodeInfo();

	/**
	 *	@brief make some self check, such as judge whether support the predefined file type.
	 */
	virtual bool selfcheck();

	/**
	 * @brief set source type.
	 * 
	 * @param type 
	 */
	void setPlaceholderSignalType(SignalType type);

	/**
	 * @brief set source target
	 * 
	 * @param target 
	 */
	void setPlaceholderSource(SignalTarget target);

private:    
    Placeholder(const Placeholder&);
    void operator=(const Placeholder&);

	bool m_queue_mode;
};
}

#include "eagleeye/processnode/Placeholder.hpp"
#endif