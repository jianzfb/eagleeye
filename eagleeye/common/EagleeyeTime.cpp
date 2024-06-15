#include "eagleeye/common/EagleeyeTime.h"
#include <stdio.h>
#include <sstream>
#include <vector>

namespace eagleeye{
long EagleeyeTime::getCurrentTime(){
	struct timeval tv;
   	gettimeofday(&tv,NULL);
   	return tv.tv_sec * 1000000 + tv.tv_usec;
}

std::string EagleeyeTime::getTimeStamp(){
	return std::to_string(getCurrentTime());
}

long EagleeyeTimeStatics::start(){
	start_time = getCurrentTime();
	return start_time;
}

long EagleeyeTimeStatics::finish(const char* format, ...){
	long finish_time = getCurrentTime();
	long cost_time = finish_time - start_time;
	total_time_cost += cost_time;
	total_run_times++;
	max_time_cost = std::max(max_time_cost, cost_time);
	min_time_cost = std::min(min_time_cost, cost_time);
	if(every_n_static == 0){
		every_n_static = 1;
	}
	if(total_run_times>=every_n_static){
        va_list args;
        va_start(args, format);

        // We might need a second shot at this, so pre-emptivly make a copy
        va_list args_copy;
        va_copy(args_copy, args);

        const int initial_buf_size = 512;
        std::vector<char> buf(initial_buf_size);

        int required_buf_size = vsnprintf(buf.data(), buf.size(), format, args);
        va_end(args);

        if (required_buf_size >= buf.size()) {
            // vsnprintf returns the number of chars we would need
            buf.resize(required_buf_size + 1);  // Allow space for '\0'
            vsnprintf(buf.data(), buf.size(), format, args_copy);
        }
        va_end(args_copy);
		EAGLEEYE_LOGI("%s, avg_cost(%d us), max_cost(%d us), min_cost(%d us), run_times(%d)",
		buf.data(), total_time_cost/total_run_times, max_time_cost, min_time_cost, every_n_static);
		total_run_times = 0;
		total_time_cost = 0;
		max_time_cost = LONG_MIN;
		min_time_cost = LONG_MAX;
	}
	return finish_time;
}

}