#include "eagleeye/common/EagleeyeLog.h"
#include <string>
#include <iostream>
#include <cstdlib>

namespace eagleeye{
#ifndef EAGLEEYE_ANDROID_APP
bool initVerbosityLevel();	
bool isVerbosityLevelEnabled (VERBOSITY_LEVEL level);

void _log_print_info (const char* tag, const char* a, const int b, const char* c, const char *format, ...){
	if (!isVerbosityLevelEnabled(L_INFO)) return; 

	printf("[%s] I [%s(%d)-%s]->\t", tag, a, b, c);
	va_list ap;

	va_start (ap, format);
	vfprintf (stdout, format, ap);
	va_end (ap);
	fflush(stdout);

	printf("\n");
}

void _log_print_debug (const char* tag, const char* a, const int b, const char* c, const char *format, ...){
	if (!isVerbosityLevelEnabled (L_DEBUG)) return;

	printf("[%s] D [%s(%d)-%s]->\t", tag, a, b, c);
	va_list ap;

	va_start (ap, format);
	vfprintf (stdout, format, ap);
	va_end (ap);
	fflush(stdout);

	printf("\n");
}

void _log_print_error (const char* tag, const char* a, const int b, const char* c, const char *format, ...){
	if (!isVerbosityLevelEnabled (L_ERROR)) return;

	printf("[%s] E [%s(%d)-%s]->\t", tag, a, b, c);
	va_list ap;

	va_start (ap, format);
	vfprintf (stdout, format, ap);
	va_end (ap);
	fflush(stdout);

	printf("\n");
}

void _log_print_verbose (const char* tag, const char* a, const int b, const char* c, const char *format, ...){
	if (!isVerbosityLevelEnabled (L_VERBOSE)) return;

	printf("[%s] E [%s(%d)-%s]->\t", tag, a, b, c);
	va_list ap;

	va_start (ap, format);
	vfprintf (stdout, format, ap);
	va_end (ap);
	fflush(stdout);

	printf("\n");
}

static bool s_need_verbosity_init = true;
static VERBOSITY_LEVEL s_verbosity_level = L_DEBUG;

bool isVerbosityLevelEnabled (VERBOSITY_LEVEL level){
	if (s_need_verbosity_init) initVerbosityLevel ();
	return level <= s_verbosity_level;  
}

bool initVerbosityLevel (){
	s_verbosity_level = L_DEBUG; // Default value
	char* vi_verbosity_level = std::getenv("EAGLEEYE_VERBOSITY_LEVEL");
	if (vi_verbosity_level)
	{
		std::string s_vi_verbosity_level (vi_verbosity_level);
		// std::transform(s_vi_verbosity_level.begin (), s_vi_verbosity_level.end (), s_vi_verbosity_level.begin (), toupper);

		if (s_vi_verbosity_level.find ("ALWAYS") != std::string::npos)          s_verbosity_level = L_ALWAYS;
		else if (s_vi_verbosity_level.find ("ERROR") != std::string::npos)      s_verbosity_level = L_ERROR;
		else if (s_vi_verbosity_level.find ("WARN") != std::string::npos)       s_verbosity_level = L_WARN;
		else if (s_vi_verbosity_level.find ("INFO") != std::string::npos)       s_verbosity_level = L_INFO;
		else if (s_vi_verbosity_level.find ("DEBUG") != std::string::npos)      s_verbosity_level = L_DEBUG;
		else if (s_vi_verbosity_level.find ("VERBOSE") != std::string::npos)    s_verbosity_level = L_VERBOSE;
		else std::cout << "Warning: invalid EAGLEEYE_VERBOSITY_LEVEL set (" << s_vi_verbosity_level << ")" << std::endl;
	}

	s_need_verbosity_init = false;
	return true;
}
#endif
}
