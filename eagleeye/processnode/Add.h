#ifndef _EAGLEEYE_Add_H_
#define _EAGLEEYE_Add_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/MetaOperation.h"
#include "eagleeye/basic/MatrixMath.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/processnode/ImageProcessNode.h"
#include "eagleeye/framework/pipeline/AnyUnit.h"
#include <string>

namespace eagleeye{
template<class SrcT, class TargetT>
class Add:public ImageProcessNode<SrcT, TargetT>{
public:
    /**
     * @brief define basic info
     * @note you must do these
     */
    typedef Add                               Self;
    typedef ImageProcessNode<SrcT, TargetT>             Superclass;

    typedef typename SrcT::MetaType						InputPixelType;
	typedef typename TargetT::MetaType					OutputPixelType;

    Add();
    virtual ~Add();

	/**
	 *	@brief execute segmentation algorithm
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

	/**
	 *	@brief make self check
	 *	@note judge whether some preliminary conditions have been satisfied.
	 */
	virtual bool selfcheck();

    /**
     * @brief Set the Parameter object
     * 
     * @param parameter 
    */
    void setParameter(int parameter);
    void getParameter(int& parameter);

    void setB(bool b);
    void getB(bool& b);

    void setAdjust(float adjust);
    void getAdjust(float& adjust);

    /**
     * @brief get class identity
     * 
     */
    EAGLEEYE_CLASSIDENTITY(Add);

private:
    /**
     * @brief Construct a new Test Increment Node object
     * @note prohibit
     */
    Add(const Add&);
	void operator=(const Add&);

    int m_parameter;
    bool m_b; 
    float m_adjust;
};
}

#include "eagleeye/processnode/Add.hpp"
#endif