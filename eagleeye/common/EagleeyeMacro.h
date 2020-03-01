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
	EAGLEEYE_UNDEFINED =-1,
	EAGLEEYE_CHAR,
	EAGLEEYE_UCHAR,
	EAGLEEYE_SHORT,
	EAGLEEYE_USHORT,
	EAGLEEYE_INT,
	EAGLEEYE_UINT,
	EAGLEEYE_FLOAT,
	EAGLEEYE_DOUBLE,
	EAGLEEYE_RGB,
	EAGLEEYE_RGBA
};

enum EagleeyeError{
	EAGLEEYE_ARG_ERROR			=-1,// parameter error
	EAGLEEYE_FILE_ERROR			=-2,// file io error
	EAGLEEYE_MEM_ERROR			=-3,// memory operation error
	EAGLEEYE_TABLE_ERROR		=-4,// table error
	EAGLEEYE_UNKNOWN_ERROR		=-5,//	unknown error
	EAGLEEYE_NO_ERROR			=0	//  no error
};

enum InterpMethod
{
	LINEAR_INTERPOLATION,
	BILINEAR_INTERPOLATION,
	SPLINE_INTERPOLATION,
	NEAREST_NEIGHBOR_INTERPOLATION
};

//#pragma warning( disable: 4251 )
//#pragma warning( disable: 4996 )

enum SignalType{
	EAGLEEYE_SIGNAL_IMAGE 	= 0,
	EAGLEEYE_SIGNAL_RECT 	= 1,
	EAGLEEYE_SIGNAL_LINE	= 2,
	EAGLEEYE_SIGNAL_POINT	= 3,
	EAGLEEYE_SIGNAL_STRING 	= 4,
	EAGLEEYE_SIGNAL_MASK	= 5
};

enum SignalTarget{
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
	EAGLEEYE_UNKNOWN_RUNTIME = -1,
	EAGLEEYE_CPU = 0,
	EAGLEEYE_GPU = 1,
	EAGLEEYE_QUALCOMM_DSP = 2,
	EAGLEEYE_QUALCOMM_NPU = 3
};
}

#endif
