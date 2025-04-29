#ifndef _EAGLEEYEMACRO_H_
#define _EAGLEEYEMACRO_H_

//#ifdef EAGLEEYE_EXPORTS
//#define EAGLEEYE_API __declspec(dllexport)
//#else
//#define EAGLEEYE_API __declspec(dllimport)
//#endif
#define EAGLEEYE_API
namespace eagleeye{
#ifndef eagleeye_eps
#define eagleeye_eps  2.2204e-16
#endif

#ifndef EAGLEEYE_PI
#define EAGLEEYE_PI 3.1415926f
#endif

#ifndef EAGLEEYE_FINF
#define EAGLEEYE_FINF 1.0e38f
#endif

#ifndef EAGLEEYE_NEAR_INF
#define EAGLEEYE_NEAR_INF 1.0e37f
#endif

#ifndef EAGLEEYE_MAX
#define EAGLEEYE_MAX(a,b) (((a)>(b))?(a):(b))
#endif

#ifndef EAGLEEYE_MIN
#define EAGLEEYE_MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef eagleeye_max
#define eagleeye_max(a,b) (((a)>(b))?(a):(b))
#endif

#ifndef eagleeye_min
#define eagleeye_min(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef eagleeye_clip
#define eagleeye_clip(a,b,c) eagleeye_min(eagleeye_max(a,b),c)
#endif

#ifndef EAGLEEYE_MAX_PATH
#define EAGLEEYE_MAX_PATH          260
#endif

#ifndef EAGLEEYE_MAX_NAME
#define EAGLEEYE_MAX_NAME			100
#endif

#ifndef EAGLEEYE_MAX_BLOCK
#define EAGLEEYE_MAX_BLOCK			400
#endif

#ifndef EAGLEEYE_MAX_STR
#define EAGLEEYE_MAX_STR			500
#endif

#ifndef EAGLEEYE_BUFFER_SIZE
#define EAGLEEYE_BUFFER_SIZE		1024
#endif

//#define bzero(a,b) memset(a,0,b)
#define EAGLEEYE_CLASSIDENTITY(NAME) \
	virtual const char* getClassIdentity() const \
	{ \
		return #NAME; \
	}
#define EAGLEEYE_GETUNITNAME \
	virtual const char* getUnitName() const \
	{ \
		return this->m_unit_name.c_str(); \
	}

#define EAGLEEYE_SETUNITNAME(NAME) \
	virtual void setUnitName(const char* unit_name=#NAME) \
	{ \
		this->m_unit_name=unit_name; \
	}

#define EAGLEEYE_OUTPUT_PORT_TYPE(TYPE,INDEX,NAME) \
	typedef TYPE OutputPort_##NAME##_Type; \
	const static int OUTPUT_PORT_##NAME = INDEX; \
	void enableOutputPort_##NAME() \
	{ \
		this->m_output_port_state[OUTPUT_PORT_##NAME] = true; \
	} \
	void disableOutputPort_##NAME() \
	{ \
		this->m_output_port_state[OUTPUT_PORT_##NAME] = false; \
	}

#define EAGLEEYE_INPUT_PORT_TYPE(TYPE,INDEX,NAME) \
	typedef TYPE InputPort_##NAME##_Type; \
	const static int INPUT_PORT_##NAME = INDEX;

#define ELEM(L, r, c, col) (L[(r) * (col) + c])  

#ifndef EAGLEEYE_SAFEFREE
#define EAGLEEYE_SAFEFREE(x) \
{\
if (x != NULL)\
{\
	free(x); \
	x = NULL; \
	}\
}
#endif

enum EagleeyeType{
	EAGLEEYE_UNDEFINED 				=-1,
	EAGLEEYE_CHAR 	   				= 0,		// int8
	EAGLEEYE_UCHAR	   				= 1,		// uint8
	EAGLEEYE_SHORT	   				= 2,		// int16
	EAGLEEYE_USHORT	   				= 3,		// uint16
	EAGLEEYE_INT	   				= 4,		// int32
	EAGLEEYE_UINT	   				= 5,		// uint32
	EAGLEEYE_FLOAT	   				= 6,		// float32
	EAGLEEYE_DOUBLE	   				= 7,
	EAGLEEYE_RGB	   				= 8,
	EAGLEEYE_RGBA	   				= 9,
	EAGLEEYE_BOOL	   				= 10,
	EAGLEEYE_STRING	   				= 11,
	EAGLEEYE_HALF_FLOAT				= 12,
	EAGLEEYE_FLOAT4					= 13,
	EAGLEEYE_TEXTURE_UINT4_RGBA		= 14,
	EAGLEEYE_YUV_I420				= 15,
	EAGLEEYE_YUV_NV21				= 16,
	EAGLEEYE_YUV_NV12				= 17,
	EAGLEEYE_INT8					= 0,
	EAGLEEYE_UINT8					= 1,
	EAGLEEYE_INT16					= 2,
	EAGLEEYE_UINT16					= 3,
	EAGLEEYE_INT32					= 4,
	EAGLEEYE_INT64					= 18,
	EAGLEEYE_FLOAT32				= 6,
	EAGLEEYE_UCHAR3					= 8,
	EAGLEEYE_UCHAR4					= 9,
	EAGLEEYE_UINT8_3				= 8,
	EAGLEEYE_UINT8_4				= 9,
};

enum EagleeyeError{
	EAGLEEYE_ARG_ERROR				=-1,	// parameter error
	EAGLEEYE_FILE_ERROR				=-2,	// file io error
	EAGLEEYE_MEM_ERROR				=-3,	// memory operation error
	EAGLEEYE_TABLE_ERROR			=-4,	// table error
	EAGLEEYE_RUNTIME_ERROR			=-5,	// runtime
	EAGLEEYE_OUT_OF_RESOURCES		=-6,
	EAGLEEYE_UNKNOWN_ERROR			=-7,	// unknown error
	EAGLEEYE_NO_ERROR				=0		//  no error
};

enum SignalCategory{
	EAGLEEYE_UNDEFINED_CATEGORY 	= -1,
	SIGNAL_CATEGORY_IMAGE 			= 0,				// 图像信号
	SIGNAL_CATEGORY_TENSOR 			= 1,				// TENSOR信号
	SIGNAL_CATEGORY_STRING 			= 2,				// 字符串信号
	SIGNAL_CATEGORY_CONTROL 		= 3,				// 控制信号
	SIGNAL_CATEGORY_STATE			= 4,				// 状态信号
	SIGNAL_CATEGORY_IMAGE_QUEUE 	= 5,				// 图像队列信号
	SIGNAL_CATEGORY_TENSOR_QUEUE 	= 6,				// TENSOR队列信号
	SIGNAL_CATEGORY_STRING_QUEUE 	= 7,				// 字符串队列信号
	SIGNAL_CATEGORY_DEFAULT			= 8,
	SIGNAL_CATEGORY_DEFAULT_QUEUE	= 9,
	SIGNAL_CATEGORY_LIST_STRING 	= 10,				// 字符串列表信号
	SIGNAL_CATEGORY_LIST_STRING_QUEUE	= 11,			
};

enum SignalType{
	EAGLEEYE_UNDEFINED_SIGNAL 		= -1,
	EAGLEEYE_SIGNAL_IMAGE 			= 0,				// Matrix<unsigned char> or Matrix<Array<unsigned char,3>> or Matrix<Array<unsigned char,4>>
	EAGLEEYE_SIGNAL_RECT 			= 1,				// Matrix<float>	xywh
	EAGLEEYE_SIGNAL_LINE			= 2,				// Matrix<float>    x1y1x2y2 
	EAGLEEYE_SIGNAL_POINT			= 3,				// Matrix<float> 	x1y1
	EAGLEEYE_SIGNAL_STRING 			= 4,				// std::string
	EAGLEEYE_SIGNAL_MASK			= 5,				// Matrix<unsigned char>
	EAGLEEYE_SIGNAL_FILE			= 6,				// std::string path
	EAGLEEYE_SIGNAL_TEXT			= 7,				// std::string
	EAGLEEYE_SIGNAL_MODEL			= 8,				// std::string path
	EAGLEEYE_SIGNAL_LANDMARK		= 9,				// Matrix<float>	x,y,z,x,y,z
	EAGLEEYE_SIGNAL_STATE			= 10,				// int
	EAGLEEYE_SIGNAL_SWITCH			= 11,				// boolean
	EAGLEEYE_SIGNAL_RGB_IMAGE 		= 12, 				// RGB image
	EAGLEEYE_SIGNAL_RGBA_IMAGE 		= 13, 				// RGBA image
	EAGLEEYE_SIGNAL_BGR_IMAGE 		= 14, 				// BGR image
	EAGLEEYE_SIGNAL_BGRA_IMAGE 		= 15, 				// RGBA image
	EAGLEEYE_SIGNAL_GRAY_IMAGE 		= 16, 				// GRAY image
	EAGLEEYE_SIGNAL_YUV_IMAGE		= 17,				// YUV
	EAGLEEYE_SIGNAL_TEXTURE			= 18,				// TEXTURE
	EAGLEEYE_SIGNAL_DET 			= 19,				// Matrix<float> x,y,w,h,label,s
	EAGLEEYE_SIGNAL_DET_EXT			= 20,				// Matrix<float> xc,yc,scale,theta,label,s
	EAGLEEYE_SIGNAL_TRACKING		= 21,				// Matrix<float> x,y,w,h,id,s
	EAGLEEYE_SIGNAL_CLS				= 22,				// Matrix<int>
	EAGLEEYE_SIGNAL_DATA			= 23,				// Matrix<float> data
	EAGLEEYE_SIGNAL_POS_2D			= 24,				//
	EAGLEEYE_SIGNAL_POS_3D			= 25,
	EAGLEEYE_SIGNAL_GROUP			= 26, 				// 
	EAGLEEYE_SIGNAL_TENSOR			= 27,
	EAGLEEYE_SIGNAL_TIMESTAMP		= 28,				// timestamp
	EAGLEEYE_SIGNAL_UCMATRIX		= 16,
	EAGLEEYE_SIGNAL_FMATRIX		 	= 19,				// 
	EAGLEEYE_SIGNAL_IMATRIX		 	= 10				// 
};

enum SignalTarget{
	EAGLEEYE_UNDEFINED_TARGET		= -1,
	EAGLEEYE_CAPTURE_STILL_IMAGE 	= 0,
	EAGLEEYE_PHOTO_GALLERY_IMAGE	= 1,
	EAGLEEYE_CAPTURE_PREVIEW_IMAGE 	= 2,
	EAGLEEYE_CAPTURE_CLICK			= 3,
	EAGLEEYE_CAPTURE_LINE			= 4,
	EAGLEEYE_CAPTURE_RECT			= 5,
	EAGLEEYE_CAPTURE_POINT			= 6,
	EAGLEEYE_CAPTURE_MASK			= 7,
	EAGLEEYE_CAPTURE_VIDEO_IMAGE	= 8
};

enum EagleeyeRuntimeType{
	EAGLEEYE_UNKNOWN_RUNTIME 		= -1,
	EAGLEEYE_CPU 					= 0,
	EAGLEEYE_GPU 					= 1,
	EAGLEEYE_QUALCOMM_DSP 			= 2,
	EAGLEEYE_QUALCOMM_NPU 			= 3
};

enum class DataFormat {
  NONE = 0, NHWC = 1, NCHW = 2, NC = 3,
  HWOI = 100, OIHW = 101, HWIO = 102, OHWI = 103,
  AUTO = 1000,
};

enum GPUPerfHint {
  PERF_DEFAULT 						= 0,
  PERF_LOW 							= 1,
  PERF_NORMAL 						= 2,
  PERF_HIGH 						= 3
};

enum GPUPriorityHint {
  PRIORITY_DEFAULT 					= 0,
  PRIORITY_LOW 						= 1,
  PRIORITY_NORMAL 					= 2,
  PRIORITY_HIGH 					= 3
};

// AFFINITY_NONE: initiate 'num_threads_hint' threads with no affinity
// scheduled.
// If 'num_threads_hint' is -1 or greater than number of available cores,
// 'num_threads_hint' will be reset to number of available cores.
// AFFINITY_BIG_ONLY: all available big cores are used, and number of threads
// is equal to numbers of available big cores.
// AFFINITY_LITTLE_ONLY: all available little cores are used, and number of
// threads is equal to numbers of available little cores.
// AFFINITY_HIGH_PERFORMANCE: initiate 'num_threads_hint' threads on different
// cores with top-num_threads_hint frequencies.
// If 'num_threads_hint' is -1 or greater than number of available cores,
// 'num_threads_hint' will be reset to number of available cores.
// AFFINITY_POWER_SAVE: initiate 'num_threads_hint' threads on different
// cores with bottom-num_threads_hint frequencies.
// If 'num_threads_hint' is -1 or greater than number of available cores,
// 'num_threads_hint' will be reset to number of available cores.
enum CPUAffinityPolicy {
  AFFINITY_NONE 					= 0,
  AFFINITY_BIG_ONLY 				= 1,
  AFFINITY_LITTLE_ONLY 				= 2,
  AFFINITY_HIGH_PERFORMANCE 		= 3,
  AFFINITY_POWER_SAVE 				= 4,
};
}

#endif
