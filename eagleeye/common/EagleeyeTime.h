#ifndef _EAGLEEYETIME_H_
#define _EAGLEEYETIME_H_
#include<sys/time.h>
#include "eagleeye/common/EagleeyeLog.h"
#include <string>

namespace eagleeye{
class EagleeyeTime{
public:
	static long getCurrentTime();	
	static std::string getTimeStamp();
};

#ifndef EAGLEEYE_TIME_LOGD
	#define EAGLEEYE_TIME_LOGD(tag, t) EAGLEEYE_LOGD("elapse time %d us at <%s> section",t,#tag)
#endif

#ifndef EAGLEEYE_TIME_START
	#define EAGLEEYE_TIME_START(tag) \
		long start_##tag##_time = EagleeyeTime::getCurrentTime();
	
	#define EAGLEEYE_TIME_END(tag) \ 
		long end_##tag##_time = EagleeyeTime::getCurrentTime(); \
		EAGLEEYE_TIME_LOGD(tag, end_##tag##_time-start_##tag##_time);

	#define EAGLEEYE_TIME_GET(tag) (end_##tag##_time-start_##tag##_time)
#endif
}

#endif