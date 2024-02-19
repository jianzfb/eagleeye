#ifndef _EAGLEEYE_LAMBDANODE_H_
#define _EAGLEEYE_LAMBDANODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Tensor.h"

namespace eagleeye
{
template<class T>
class LambdaNode:public AnyNode{
public:
    typedef LambdaNode              Self;
    typedef AnyNode                 Superclass;    

    EAGLEEYE_CLASSIDENTITY(LambdaNode);

    LambdaNode(std::function<void(Tensor&, std::vector<AnySignal*>, std::vector<AnySignal*>&)> lambda_func){
        m_lambda_func = lambda_func;
        this->setNumberOfOutputSignals(1);
        this->setOutputPort(new T(), 0);
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
        this->m_lambda_func(m_cache, input_signals, output_signals);
    }

private:
    LambdaNode(const LambdaNode&);
    void operator=(const LambdaNode&);

    std::function<void(Tensor&, std::vector<AnySignal*>, std::vector<AnySignal*>&)> m_lambda_func;
    Tensor m_cache;
};


template<class T1, class T2>
class Lambda2Node:public AnyNode{
public:
    typedef Lambda2Node              Self;
    typedef AnyNode                 Superclass;    

    EAGLEEYE_CLASSIDENTITY(Lambda2Node);

    Lambda2Node(std::function<void(Tensor&, std::vector<AnySignal*>, std::vector<AnySignal*>&)> lambda_func){
        m_lambda_func = lambda_func;
        this->setNumberOfOutputSignals(2);
        this->setOutputPort(new T1(), 0);
        this->setOutputPort(new T2(), 1);
    }
    virtual ~Lambda2Node(){}

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
        this->m_lambda_func(m_cache, input_signals, output_signals);
    }

private:
    Lambda2Node(const Lambda2Node&);
    void operator=(const Lambda2Node&);

    std::function<void(Tensor&, std::vector<AnySignal*>, std::vector<AnySignal*>&)> m_lambda_func;
    Tensor m_cache;
};
}
#endif