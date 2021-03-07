#ifndef _EAGLEEYE_SHAPE_NODE_H_
#define _EAGLEEYE_SHAPE_NODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/render/RenderNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include <GLES3/gl3.h>

namespace eagleeye
{
class ShapeNode:public RenderNode{
public:
    typedef ShapeNode                   Self;
    typedef RenderNode                  Superclass;

    EAGLEEYE_CLASSIDENTITY(ShapeNode);

    ShapeNode();
    virtual ~ShapeNode();

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    /**
     * @brief init gl 
     */ 
    virtual void init();

private:
    ShapeNode(const ShapeNode&);
    void operator=(const ShapeNode&);
};    
} // namespace eagleeye

#endif