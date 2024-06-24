#ifndef _REDERREGION_H_
#define _REDERREGION_H_

#include "eagleeye/framework/pipeline/AnyNode.h"

namespace eagleeye
{
class RenderRegionNode: public AnyNode{
public:
    typedef RenderRegionNode                      Self;
    typedef AnyNode                         Superclass;

    EAGLEEYE_CLASSIDENTITY(RenderRegionNode);

    RenderRegionNode();
    virtual ~RenderRegionNode();

	virtual void executeNodeInfo() override;

private:
    RenderRegionNode(const RenderRegionNode&);
    void operator=(const RenderRegionNode&);
};

}
#endif _REDERREGION_H_