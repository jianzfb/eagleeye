#ifndef _EAGLEEYETIME_H_
#define _EAGLEEYETIME_H_
#include <sys/time.h>
#include <limits.h>
#include "eagleeye/common/EagleeyeLog.h"
#include <string>

namespace eagleeye{
class EagleeyeTime{
public:
	static long getCurrentTime();	
	static std::string getTimeStamp();
};

class EagleeyeTimeStatics : public EagleeyeTime{
public: 	
	EagleeyeTimeStatics() = default;
	EagleeyeTimeStatics(long every_n): every_n_static(every_n){}
	~EagleeyeTimeStatics() = default;
	
	long start();
	long finish(const char* format, ...);

private:
	long total_time_cost = 0;
	long total_run_times = 0;
	long max_time_cost = LONG_MIN;
	long min_time_cost = LONG_MAX;
	long every_n_static = 150;
	long start_time = 0;
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