#ifndef _EAGLEEYE_LAMBDANODE_H_
#define _EAGLEEYE_LAMBDANODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"

namespace eagleeye
{
template<class T>
class LambdaNode:public AnyNode{
public:
    typedef LambdaNode              Self;
    typedef AnyNode                 Superclass;    

    EAGLEEYE_CLASSIDENTITY(LambdaNode);

    LambdaNode(std::function<void(std::vector<AnySignal*>, std::vector<AnySignal*>&)> lambda_func){
        m_lambda_func = lambda_func;
        this->setNumberOfOutputSignals(1);
        for(int signal_i=0; signal_i<1; ++signal_i){
            this->setOutputPort(new T(), signal_i);
        }
    }
    virtual ~LambdaNode(){}

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo(){
        std::vector<AnySignal*> input_signals;
        int signal_num = this->getNumberOfInputSignals();
        for(int signal_i=0; signal_i<signal_num; ++signal_i){
            input_signals.push_back(this->getInputPort(signal_i));
        }

        std::vector<AnySignal*> output_signals;
        signal_num = this->getNumberOfOutputSignals();
        for(int signal_i=0; signal_i<signal_num; ++signal_i){
            output_signals.push_back(this->getOutputPort(signal_i));
        }
        this->m_lambda_func(input_signals, output_signals);
    }

private:
    LambdaNode(const LambdaNode&);
    void operator=(const LambdaNode&);

    std::function<void(std::vector<AnySignal*>, std::vector<AnySignal*>&)> m_lambda_func;
};
}
#endif