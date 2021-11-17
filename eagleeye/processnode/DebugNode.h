#ifndef _EAGLEEYE_DEBUGNODE_H_
#define _EAGLEEYE_DEBUGNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeTime.h"
#include <string>

namespace eagleeye{
class DebugNode: public AnyNode{
public:
    typedef DebugNode           Self;
    typedef AnyNode             Superclass;

    DebugNode(int input_port_num, int output_port_num);
    virtual ~DebugNode();

    /**
	 *	@brief Get class identity
	 */
	EAGLEEYE_CLASSIDENTITY(DebugNode);

    /**
	 * @brief execute control
	 * 
	 */
	virtual void executeNodeInfo();

    /**
     * @brief Set the Output Port Category Type object
     * 
     */
    void setPortCategoryType(int port_index, SignalCategory port_category, EagleeyeType tt=EAGLEEYE_UNDEFINED);

    /**
     * @brief reset node
     * 
     */
    virtual void reset();

    /**
     * @brief init node
     * 
     */
    virtual void init();

private:
    DebugNode(const DebugNode&);
    void operator=(const DebugNode&);

    int m_reset_times;
    int m_init_times;
    int m_execute_times;
};
}
#endif