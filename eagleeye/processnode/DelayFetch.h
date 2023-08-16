#ifndef _EAGLEEYE_DELAYFETCH_H_
#define _EAGLEEYE_DELAYFETCH_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"


namespace eagleeye{
class DelayFetch:public AnyNode, DynamicNodeCreator<DelayFetch>{
public:
    typedef DelayFetch          Self;
    typedef AnyNode             Superclass;

    /**
	 *	@brief Get class identity
	 */
    EAGLEEYE_CLASSIDENTITY(DelayFetch);

    /**
     *  @brief constructor/destructor
     */
    DelayFetch();
    virtual ~DelayFetch();

    /**
     * @brief add/set input port
     * 
     * @param sig 
     */
	virtual void addInputPort(AnySignal* sig);
	virtual void setInputPort(AnySignal* sig,int index=0);

    /**
	 * @brief execute node 
	 * 
	 */
    virtual void executeNodeInfo();

    /**
     * @brief set/get delay_time
     */
    void setDelayTime(int delay_time);
    void getDelayTime(int& delay_time);

private:
    DelayFetch(const DelayFetch&);
    void operator=(const DelayFetch&);

    int m_delay_count;
    int m_count;
};
}
#endif