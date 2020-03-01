#ifndef _EAGLEEYE_SKYSEGNODE_H_
#define _EAGLEEYE_SKYSEGNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/processnode/ImageProcessNode.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/engine/proxy_run.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/processnode/SegNode.h"


namespace eagleeye{
class SkySegNode:public SegNode{
public:
	/**
	 *	@brief define some basic type
	 *	@note you must do these
	 */
	typedef SkySegNode		        Self;
	typedef SegNode	                Superclass;

    SkySegNode(EagleeyeRuntimeType runtime);
    virtual ~SkySegNode();

    /**
	 *	@brief get class identity
	 */
	EAGLEEYE_CLASSIDENTITY(SkySegNode);


    /**
	 *	@brief execute goturn algorithm
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

private:
    SkySegNode(const SkySegNode&);
    void operator=(const SkySegNode&);

}; 
}
#endif