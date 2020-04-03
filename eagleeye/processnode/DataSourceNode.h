#ifndef _EAGLEEYE_DATASOURCE_H_
#define _EAGLEEYE_DATASOURCE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/processnode/ImageIONode.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/common/EagleeyeIO.h"

namespace eagleeye{
template<class T>
class DataSourceNode:public ImageIONode<T>{
public:
    typedef DataSourceNode                  Self;
    typedef AnyNode                         Superclass;

    typedef typename T::MetaType            OutputPixelType;

	/**
	 * @brief Construct a new Data Source Node object
	 * 
	 * @param unit_name 
	 * @param type 
	 * @param source 
	 * @param queue_mode 
	 */
    DataSourceNode(bool queue_mode=false);
    virtual ~DataSourceNode();

    /**
	 *	@brief get class identity
	 */
	EAGLEEYE_CLASSIDENTITY(DataSourceNode);

    /**
	 *	@brief define output port image signal type
	 */
	EAGLEEYE_OUTPUT_PORT_TYPE(T,	0,	IMAGE_DATA);

	/**
	 * @brief Set the Source Type object
	 * 
	 * @param type 
	 */
	void setSourceType(SignalType type);

	/**
	 * @brief Set the Source Target object
	 * 
	 * @param source 
	 */
	void setSourceTarget(SignalTarget target);

	/**
     * @brief push data to data source
     * 
     * @param data 
     */
    void setSourceData(Matrix<OutputPixelType> data);

	/**
	 *	@brief 
	 */
	virtual void executeNodeInfo();

	/**
	 *	@brief make some self check, such as judge whether support the predefined file type.
	 */
	virtual bool selfcheck();

	/**
     * @brief (pre/post) exit
     * 
     */
    virtual void preexit();
    virtual void postexit();

private:    
    DataSourceNode(const DataSourceNode&);
    void operator=(const DataSourceNode&);

	bool m_queue_mode;
};
}

#include "eagleeye/processnode/DataSourceNode.hpp"
#endif