#ifndef _EAGLEEYELOG_H_
#define _EAGLEEYELOG_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeStr.h"
#include <stdarg.h>
#include <utility>

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
	#define EAGLEEYE_ASSERT(condition, ...) ((void)0)
#endif


// EAGLEEYE_CHECK/EAGLEEYE_ASSERT dies with a fatal error if condition is not true.
// EAGLEEYE_ASSERT is controlled by NDEBUG ('-c opt' for bazel) while EAGLEEYE_CHECK
// will be executed regardless of compilation mode.
// Therefore, it is safe to do things like:
//    EAGLEEYE_CHECK(fp->Write(x) == 4)
//    EAGLEEYE_CHECK(fp->Write(x) == 4, "Write failed")
// which are not safe for EAGLEEYE_ASSERT.
#define EAGLEEYE_CHECK(condition, ...) \
  if (!(condition)) \
  	EAGLEEYE_LOGE("Check failed: "#condition" %s",makeString(__VA_ARGS__).c_str());

#define EAGLEEYE_ASSERT(condition, ...) \
  if (!(condition)) \
  	EAGLEEYE_LOGE("Assert failed: " #condition " %s",makeString(__VA_ARGS__).c_str())	 

template <typename T>
T &&CheckNotNull(const char *file, int line, const char *exprtext, T &&t) {
  if (t == nullptr) {
	EAGLEEYE_LOGE("%s %d %s",file, line, exprtext);
  }
  return std::forward<T>(t);
}

#define EAGLEEYE_CHECK_NOTNULL(val) \
  CheckNotNull(__FILE__, __LINE__, \
        "'" #val "' Must not be NULL", (val))

#define EAGLEEYE_NOT_IMPLEMENTED EAGLEEYE_CHECK(false, "not implemented")

#define EAGLEEYE_RETURN_IF_ERROR(stmt)                           \
  {                                                          \
    EagleeyeError status = (stmt);                              \
    if (status != EagleeyeError::EAGLEEYE_NO_ERROR) {                \
	  EAGLEEYE_LOGE(#stmt" failed with error: %d", int(status)); \
      return status;                                         \
    }                                                        \
  }

#endif
