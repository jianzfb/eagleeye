#ifndef _EAGLEEYE_ADAPTORNODE_H_
#define _EAGLEEYE_ADAPTORNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"

namespace eagleeye{
/**
 * @brief define condition function type
 * 
 */
typedef void (*AdaptorFunc)(std::vector<AnySignal*> input_sigs, std::vector<AnySignal*> output_sigs);

class AdaptorNode:public AnyNode{
public:
    typedef AdaptorNode                     Self;
    typedef AnyNode                         Superclass;

    AdaptorNode(int input_sigs_num, int output_sigs_num, AdaptorFunc func);
    virtual ~AdaptorNode();

    EAGLEEYE_CLASSIDENTITY(AdaptorNode);
    
    template<class TargetT>
    void configureOutputSignal(int port_index){
        assert(port_index < this->getNumberOfOutputSignals());
        // 设置输出端口(端口0)及携带数据类型(TargetT)
        this->setOutputPort(new TargetT, port_index);
    }
    
    /**
	 * @brief execute image process
	 * 
	 */
	virtual void executeNodeInfo();

    /**
	 *	@brief make self check
	 *	@note judge whether some preliminary conditions have been satisfied.
	 */
	virtual bool selfcheck(){
        return true;
    }

private:
	AdaptorNode(const AdaptorNode&);
	void operator=(const AdaptorNode&);

    AdaptorFunc m_adaptor_func;
};
}
#endif