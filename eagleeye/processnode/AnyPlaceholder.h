#ifndef _EAGLEEYE_ANYPLACEHOLDER_H_
#define _EAGLEEYE_ANYPLACEHOLDER_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"

namespace eagleeye
{
class AnyPlaceholder: public AnyNode{
public:
    typedef AnyPlaceholder                  Self;
    typedef AnyNode                         Superclass;

    AnyPlaceholder();
    virtual ~AnyPlaceholder();

    /**
	 *	@brief get class identity
	 */
	EAGLEEYE_CLASSIDENTITY(AnyPlaceholder);

    /**
	 *	@brief execute
	 */
	virtual void executeNodeInfo();

    /**
     * @brief Set the Signal Type object
     * 
     * @param type 
     */
    void setSignalType(SignalType type);

    /**
     * @brief Set the Source object
     * 
     * @param target 
     */
    void setSource(SignalTarget target);

    /**
     * @brief use sig, make one same signal
     * 
     * @param sig 
     */
    void generate(AnySignal* sig);

    /**
	 *	@brief make some self check, such as judge whether support the predefined file type.
	 */
	virtual bool selfcheck();

	/**
     * @brief (pre/post) exit
     * 
     */
    virtual void preexit();
    virtual void postexit();

	/**
	 * @brief reset 
	 * 
	 */
	virtual void reset();

private:    
    AnyPlaceholder(const AnyPlaceholder&);
    void operator=(const AnyPlaceholder&);
};    
} // namespace eagleeye

#endif