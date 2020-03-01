#ifndef _EAGLEEYETIMESTAMP_H_
#define _EAGLEEYETIMESTAMP_H_

#include "eagleeye/common/EagleeyeMacro.h"

namespace eagleeye
{
class EAGLEEYE_API EagleeyeTimeStamp
{
public:
	/** Constructor must remain public because classes instantiate
	*	TimeStamps implicitly in their construction.*/
	EagleeyeTimeStamp(){m_modified_time=0;}

	/** Set this objects time to the current time. The current time is just a
	* monotonically increasing unsigned long integer. It is possible for this
	*	number to wrap around back to zero. This should only happen for 
	*	processes that have been running for a very long time, while constantly
	*	changing objects within the program. When this does occur, the typical
	*	consequence should be that some filters will update themselves when
	*	really they don't need to.*/
	void modified();

	/**
	 * @brief frozen timestamp = 0
	 * 
	 */
	void frozen(){
		m_modified_time = 0;
	}

	static bool isCrossBorder(){
		return m_cross_border;
	}
	static void resetCrossBorder(){
		m_cross_border = false;
	}

	/** Return this object's Modified time.*/
	unsigned long getMTime() const
	{
		return m_modified_time;
	}

	/** Support comparisons of time stamp objects directly.*/
	bool operator>(EagleeyeTimeStamp& ts)
	{
		return (m_modified_time>ts.m_modified_time);
	}
	bool operator<(EagleeyeTimeStamp& ts)
	{
		return(m_modified_time<ts.m_modified_time);
	}
private:
	unsigned long m_modified_time;
	static bool m_cross_border;
};
}

#endif
