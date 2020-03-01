#ifndef _EAGLEEYE_IncrementOrAddNode_H_
#define _EAGLEEYE_IncrementOrAddNode_H_
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
class IncrementOrAddNode:public ImageProcessNode<SrcT, TargetT>{
public:
    /**
     * @brief define basic info
     * @note you must do these
     */
    typedef IncrementOrAddNode                               Self;
    typedef ImageProcessNode<SrcT, TargetT>             Superclass;

    typedef typename SrcT::MetaType						InputPixelType;
	typedef typename TargetT::MetaType					OutputPixelType;

    IncrementOrAddNode(int input_port_num=1, int output_port_num=1);
    virtual ~IncrementOrAddNode();

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
    EAGLEEYE_CLASSIDENTITY(IncrementOrAddNode);

private:
    /**
     * @brief Construct a new Test Increment Node object
     * @note prohibit
     */
    IncrementOrAddNode(const IncrementOrAddNode&);
	void operator=(const IncrementOrAddNode&);

    int m_parameter;
    bool m_b; 
    float m_adjust;
};
}

#include "eagleeye/processnode/IncrementOrAddNode.hpp"
#endif