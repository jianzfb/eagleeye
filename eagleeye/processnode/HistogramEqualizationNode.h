#ifndef _EAGLEEYE_HISTOGRAMEQUALIZATIONNODE_H_
#define _EAGLEEYE_HISTOGRAMEQUALIZATIONNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"


namespace eagleeye{
class HistogramEqualizationNode:public AnyNode, DynamicNodeCreator<HistogramEqualizationNode>{
public:
    typedef HistogramEqualizationNode           Self;
    typedef AnyNode                             Superclass;

    HistogramEqualizationNode();
    virtual ~HistogramEqualizationNode();

    EAGLEEYE_CLASSIDENTITY(HistogramEqualizationNode);

    /**
	 *	@brief execute histogram equalization
     *  @note 
	 */
	virtual void executeNodeInfo();

private:
    /**
     * @brief Construct a new HistogramEqualizationNode Node object
     * @note prohibit
     */
    HistogramEqualizationNode(const HistogramEqualizationNode&);
	void operator=(const HistogramEqualizationNode&);
};
}
#endif