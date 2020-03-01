#include "eagleeye/common/EagleeyeTime.h"
#include <stdio.h>

namespace eagleeye{
long EagleeyeTime::getCurrentTime(){
	struct timeval tv;
   	gettimeofday(&tv,NULL);
   	return tv.tv_sec * 1000000 + tv.tv_usec;
}
}