#include "eagleeye/common/EagleeyeTime.h"
#include <stdio.h>
#include <sstream>

namespace eagleeye{
long EagleeyeTime::getCurrentTime(){
	struct timeval tv;
   	gettimeofday(&tv,NULL);
   	return tv.tv_sec * 1000000 + tv.tv_usec;
}

std::string EagleeyeTime::getTimeStamp(){
	std::stringstream ss;
	ss<<getCurrentTime();
	std::string stamp = "timestamp_"+ss.str();
	return stamp;
}
}