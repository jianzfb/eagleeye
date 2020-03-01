#ifndef _IMAGEPROCESSNODE_H_
#define _IMAGEPROCESSNODE_H_

#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace eagleeye
{
template<class SrcT,class TargetT>
class ImageProcessNode:public AnyNode
{
public:
	/**
	 *	@brief define some basic type
	 *	@note you must do these
	 */
	typedef ImageProcessNode							Self;
	typedef AnyNode										Superclass;

	typedef typename SrcT::MetaType						InputPixelType;
	typedef typename TargetT::MetaType					OutputPixelType;


	ImageProcessNode();
	virtual ~ImageProcessNode();

	/**
	 *	@brief get class identity
	 */
	EAGLEEYE_CLASSIDENTITY(ImageProcessNode);

	/**
	 * @brief execute image process
	 * 
	 */
	virtual void executeNodeInfo() = 0;

	/**
	 *	@brief get output image
	 */
	Matrix<OutputPixelType> getOutputImage(unsigned int index=0);

	/**
	 *	@brief get input image signal
	 */
	SrcT* getInputImageSignal(unsigned int index=0);
	const SrcT* getInputImageSignal(unsigned int index=0) const;

	/**
	 *	@brief get output image signal
	 */
	TargetT* getOutputImageSignal(unsigned int index=0);
	const TargetT* getOutputImageSignal(unsigned int index=0) const;
	
	/**
	 * @brief overload base class addInputPort and add signal control
	 * 
	 * @param sig 
	 */
	virtual void addInputPort(AnySignal* sig);

	/**
	 * @brief overload base class setInputPort and add signal control
	 * 
	 * @param sig 
	 * @param index 
	 */
	virtual void setInputPort(AnySignal* sig,int index=0);
	
	/**
	 * @brief overload base class setOutputPort and add signal control
	 * 
	 * @param sig 
	 * @param index 
	 */
	virtual void setOutputPort(AnySignal* sig,int index=0);

protected:
	/**
	 *	@brief make one output signal
	 */
	virtual AnySignal* makeOutputSignal();

	/**
	 * @brief all input signal is ok
	 * 
	 * @return true 
	 * @return false 
	 */
	bool isAllInputSignalOK();

private:
	ImageProcessNode(const ImageProcessNode&);
	void operator=(const ImageProcessNode&);
};
}

#include "eagleeye/processnode/ImageProcessNode.hpp"
#endif