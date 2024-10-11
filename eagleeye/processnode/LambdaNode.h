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

template<class T1, class T2, class T3>
class Lambda3Node:public AnyNode{
public:
    typedef Lambda3Node              Self;
    typedef AnyNode                 Superclass;    

    EAGLEEYE_CLASSIDENTITY(Lambda3Node);

    Lambda3Node(std::function<void(Tensor&, std::vector<AnySignal*>, std::vector<AnySignal*>&)> lambda_func){
        m_lambda_func = lambda_func;
        this->setNumberOfOutputSignals(3);
        this->setOutputPort(new T1(), 0);
        this->setOutputPort(new T2(), 1);
        this->setOutputPort(new T3(), 2);
    }
    virtual ~Lambda3Node(){}

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
    Lambda3Node(const Lambda3Node&);
    void operator=(const Lambda3Node&);

    std::function<void(Tensor&, std::vector<AnySignal*>, std::vector<AnySignal*>&)> m_lambda_func;
    Tensor m_cache;
};

class LambdaANode:public AnyNode{
public:
    typedef LambdaANode             Self;
    typedef AnyNode                 Superclass;    

    EAGLEEYE_CLASSIDENTITY(LambdaANode);

    LambdaANode(std::function<void(std::vector<Tensor>&, std::vector<AnySignal*>, std::vector<AnySignal*>&)> lambda_func){
        m_lambda_func = lambda_func;
    }
    virtual ~LambdaANode(){}

    template<class T>
    void append(T* out_sig){
        int num = this->getNumberOfOutputSignals();
        this->setNumberOfOutputSignals(num + 1);
        this->setOutputPort(out_sig, num);
    }

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo(){
        // 汇集输入信号
        std::vector<AnySignal*> input_signals;
        int signal_num = this->getNumberOfInputSignals();
        for(int signal_i=0; signal_i<signal_num; ++signal_i){
            input_signals.push_back(this->getInputPort(signal_i));
        }

        // 汇集输出信号
        std::vector<AnySignal*> output_signals;
        signal_num = this->getNumberOfOutputSignals();
        for(int signal_i=0; signal_i<signal_num; ++signal_i){
            output_signals.push_back(this->getOutputPort(signal_i));
        }

        // 执行lambda
        this->m_lambda_func(m_caches, input_signals, output_signals);
    }

private:
    LambdaANode(const LambdaANode&);
    void operator=(const LambdaANode&);

    std::function<void(std::vector<Tensor>&, std::vector<AnySignal*>, std::vector<AnySignal*>&)> m_lambda_func;
    std::vector<Tensor> m_caches;
};

template<class T1, class T2, class T3, class T4>
class Lambda4Node:public AnyNode{
public:
    typedef Lambda4Node              Self;
    typedef AnyNode                 Superclass;    

    EAGLEEYE_CLASSIDENTITY(Lambda4Node);

    Lambda4Node(std::function<void(Tensor&, std::vector<AnySignal*>, std::vector<AnySignal*>&)> lambda_func){
        m_lambda_func = lambda_func;
        this->setNumberOfOutputSignals(4);
        this->setOutputPort(new T1(), 0);
        this->setOutputPort(new T2(), 1);
        this->setOutputPort(new T3(), 2);
        this->setOutputPort(new T4(), 3);
    }
    virtual ~Lambda4Node(){}

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
    Lambda4Node(const Lambda4Node&);
    void operator=(const Lambda4Node&);

    std::function<void(Tensor&, std::vector<AnySignal*>, std::vector<AnySignal*>&)> m_lambda_func;
    Tensor m_cache;
};

}
#endif