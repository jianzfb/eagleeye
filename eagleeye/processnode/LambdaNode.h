#ifndef _EAGLEEYE_LAMBDANODE_H_
#define _EAGLEEYE_LAMBDANODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/EmptySignal.h"

namespace eagleeye
{
class LambdaNode:public AnyNode{
public:
    typedef LambdaNode              Self;
    typedef AnyNode                 Superclass;    

    EAGLEEYE_CLASSIDENTITY(LambdaNode);

    LambdaNode(std::function<void(std::vector<AnySignal*>, std::vector<AnySignal*>&)> lambda_func, int input_signal_num, int output_signal_num){
        m_lambda_func = lambda_func;
        this->setNumberOfInputSignals(input_signal_num);
        this->setNumberOfOutputSignals(output_signal_num);
        for(int signal_i=0; signal_i<output_signal_num; ++signal_i){
            this->setOutputPort(new EmptySignal(), signal_i);
        }
        m_is_init = false;
    }
    virtual ~LambdaNode(){}

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo(){
        std::vector<AnySignal*> input_signals;
        int signal_num = this->getNumberOfOutputSignals();
        for(int signal_i=0; signal_i<signal_num; ++signal_i){
            input_signals.push_back(this->getInputPort(signal_i));
        }

        std::vector<AnySignal*> output_signals;
        this->m_lambda_func(input_signals, output_signals);

        signal_num = output_signals.size();
        if(!m_is_init){
            for(int signal_i=0; signal_i<signal_num; ++signal_i){
                this->setOutputPort(output_signals[signal_i]->make(), signal_i);
            }
            m_is_init = true;
        }

        for(int signal_i=0; signal_i<signal_num; ++signal_i){
            this->getOutputPort(signal_i)->copy(output_signals[signal_i]);
        }
    }

private:
    LambdaNode(const LambdaNode&);
    void operator=(const LambdaNode&);

    std::function<void(std::vector<AnySignal*>, std::vector<AnySignal*>&)> m_lambda_func;
    bool m_is_init;
};
}
#endif