#ifndef _EAGLEEYE_RENDERMAPTOCPU_H_
#define _EAGLEEYE_RENDERMAPTOCPU_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/render/RenderNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include <GLES3/gl3.h>

namespace eagleeye
{
class RenderMapToCPU:public RenderNode{
public:
    typedef RenderMapToCPU                      Self;
    typedef RenderNode                      Superclass;

    EAGLEEYE_CLASSIDENTITY(RenderMapToCPU);

    RenderMapToCPU();
    virtual ~RenderMapToCPU();

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

private:
    RenderMapToCPU(const RenderMapToCPU&);
    void operator=(const RenderMapToCPU&);

    Matrix<Array<unsigned char, 4>> m_temp;
};    
} // namespace eagleeye

#endif