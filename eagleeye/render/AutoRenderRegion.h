#ifndef _EAGLEEYE_AUTORENDERREGION_H_
#define _EAGLEEYE_AUTORENDERREGION_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/render/RenderNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"


namespace eagleeye{
class AutoRenderRegion:public RenderNode, DynamicNodeCreator<AutoRenderRegion>{
public:
    typedef AutoRenderRegion                     Self;
    typedef RenderNode                              Superclass;

    EAGLEEYE_CLASSIDENTITY(AutoRenderRegion);

    AutoRenderRegion();
    virtual ~AutoRenderRegion();

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

private:
    AutoRenderRegion(const AutoRenderRegion&);
    void operator=(const AutoRenderRegion&);
};    
} // namespace eagleeye


#endif