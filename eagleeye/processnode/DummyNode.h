#ifndef _EAGLEEYE_DUMMYNODE_H_
#define _EAGLEEYE_DUMMYNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"

namespace eagleeye{
template<typename SrcT, typename TargetT>
class DummyNode:public AnyNode{
public:
    DummyNode(){
        this->setNumberOfInputSignals(1);
        // 设置输出端口（拥有1个输出端口）
        this->setNumberOfOutputSignals(1);
        // 设置输出端口(端口0)及携带数据类型(TargetT)
        this->setOutputPort(new TargetT, 0);
    }
    virtual ~DummyNode(){}

    /**
	 *	@brief Get class identity
	 */
	EAGLEEYE_CLASSIDENTITY(DummyNode);

    void setFixedOutput(typename TargetT::DataType data){
        this->m_fixed_output = data;
    }

    /**
	 * @brief execute control
	 * 
	 */
	virtual void executeNodeInfo(){
        EAGLEEYE_LOGD("run in dummy node %s", this->getUnitName());
        EAGLEEYE_LOGD("input data");
        SrcT* input_sig = (SrcT*)(this->getInputPort(0));
        typename SrcT::DataType input = input_sig->getData();
        std::cout<<input;

        EAGLEEYE_LOGD("output data");
        TargetT* output_sig = (TargetT*)(this->getOutputPort(0));
        output_sig->setData(this->m_fixed_output);
        typename TargetT::DataType output = output_sig->getData();
        std::cout<<output;
    }

private:
    private:
    DummyNode(const DummyNode&);
    void operator=(const DummyNode&);

    typename TargetT::DataType m_fixed_output;
};
}
#endif