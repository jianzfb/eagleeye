#ifndef _EAGLEEYELOG_H_
#define _EAGLEEYELOG_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include <stdarg.h>

#ifndef EAGLEEYE_LOG_TAG
#define EAGLEEYE_LOG_TAG  "eagleeye"
#endif

#ifdef EAGLEEYE_ENABLE_LOG
#ifdef EAGLEEYE_ANDROID_APP
#include <android/log.h>
#define EAGLEEYE_LOGI(...) __android_log_print(ANDROID_LOG_INFO,EAGLEEYE_LOG_TAG,__VA_ARGS__)
#define EAGLEEYE_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, EAGLEEYE_LOG_TAG, __VA_ARGS__)
#define EAGLEEYE_LOGE(...) __android_log_print(ANDROID_LOG_ERROR,EAGLEEYE_LOG_TAG,__VA_ARGS__)
#else
namespace eagleeye{
enum VERBOSITY_LEVEL{
	L_ALWAYS,
	L_ERROR,
	L_WARN,
	L_INFO,
	L_DEBUG,
	L_VERBOSE
};

void _log_print_info(const char* tag, const char *format, ...);
void _log_print_debug(const char* tag, const char *format, ...);
void _log_print_error(const char* tag, const char *format, ...);

/** initialize verbosity level. */
bool initVerbosityLevel ();

/** is verbosity level enabled? */
bool isVerbosityLevelEnabled (VERBOSITY_LEVEL severity);
}
#define EAGLEEYE_LOGI(...) eagleeye::_log_print_info(EAGLEEYE_LOG_TAG, __VA_ARGS__)
#define EAGLEEYE_LOGD(...) eagleeye::_log_print_debug(EAGLEEYE_LOG_TAG, __VA_ARGS__)
#define EAGLEEYE_LOGE(...) eagleeye::_log_print_error(EAGLEEYE_LOG_TAG,__VA_ARGS__)

#endif
#else
	#define EAGLEEYE_LOGI(...) 
	#define EAGLEEYE_LOGD(...) 
	#define EAGLEEYE_LOGE(...) 
#endif
#endif
