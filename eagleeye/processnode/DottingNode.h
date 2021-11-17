#ifndef _EAGLEEYE_DOTTINGNODE_H_
#define _EAGLEEYE_DOTTINGNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include <string>


namespace eagleeye
{
class DottingNode: public AnyNode{
public:
    typedef DottingNode             Self;
    typedef AnyNode             Superclass;

    EAGLEEYE_CLASSIDENTITY(DottingNode);

    DottingNode(std::string dotting_str="");
    virtual ~DottingNode();

    void setDottingStr(std::string dotting_str);

    virtual void executeNodeInfo();

    void addInputPort(AnySignal* sig);
    void setInputPort(AnySignal* sig,int index);

private:
    DottingNode(const DottingNode&);
    void operator=(const DottingNode&);

    std::string m_dotting_str;
};
} // namespace eagleeye


#endif