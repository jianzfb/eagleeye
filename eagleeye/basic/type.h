#ifndef _EAGLEEYE_TYPE_H_
#define _EAGLEEYE_TYPE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Array.h"
#include <limits.h>
#include <float.h>

namespace eagleeye
{
class TypeInfo{
public:
    static inline int64_t getElemSize(EagleeyeType type){
        int64_t elem_size = 0;
        switch (type){
        case EAGLEEYE_CHAR:
        case EAGLEEYE_UCHAR:
            elem_size = sizeof(unsigned char);
            break;
        case EAGLEEYE_FLOAT:
            elem_size = sizeof(float);
            break;
        case EAGLEEYE_INT:
        case EAGLEEYE_UINT:
            elem_size = sizeof(int32_t);
            break;
        case EAGLEEYE_SHORT:
        case EAGLEEYE_USHORT:
            elem_size = sizeof(int16_t);
            break;
        case EAGLEEYE_DOUBLE:
            elem_size = sizeof(double);
            break;
        case EAGLEEYE_RGB:
            elem_size = sizeof(unsigned char)*3;
            break;
        case EAGLEEYE_RGBA:
            elem_size = sizeof(unsigned char)*4;
            break;            
        case EAGLEEYE_BOOL:
            elem_size = sizeof(bool);
            break;
        case EAGLEEYE_FLOAT4:
            elem_size = sizeof(float)*4;
            break;
        default:
            elem_size = 0;
            break;
        }
        return elem_size;
    }
};

/**
 *	@brief trait atomic type
 */
template<typename T>
class TypeTrait
{
public:
	typedef typename T::ElemType	Type;
	static const int size			= sizeof(T) / sizeof(Type);

	static inline T minval()
	{
		T data;
		for (int i = 0; i < size; ++i)
		{	
			data[i] = TypeTrait<Type>::minval();
		}

		return data;
	}
	static inline T maxval()
	{
		T data;
		for (int i = 0; i < size; ++i)
		{	
			data[i] = TypeTrait<Type>::maxval();
		}

		return data;
	}

	static const EagleeyeType type = EAGLEEYE_UNDEFINED;
};

template<>
class TypeTrait<char>
{
public:
	typedef char					Type;
	static const int size			= 1;

	static inline Type minval()
	{
		return CHAR_MIN;
	}
	static inline Type maxval()
	{
		return CHAR_MAX;
	}

	static const EagleeyeType type = EAGLEEYE_CHAR;
};

template<>
class TypeTrait<unsigned char>
{
public:
	typedef unsigned char			Type;
	static const int size			= 1;

	static inline Type minval()
	{
		return 0;
	}
	static inline Type maxval()
	{
		return UCHAR_MAX;
	}

	static const EagleeyeType type = EAGLEEYE_UCHAR;
};

template<>
class TypeTrait<short>
{
public:
	typedef short					Type;
	static const int size			= 1;

	static inline Type minval()
	{
		return SHRT_MIN;
	}
	static inline Type maxval()
	{
		return SHRT_MAX;
	}

	static const EagleeyeType type=EAGLEEYE_SHORT;
};

template<>
class TypeTrait<unsigned short>
{
public:
	typedef unsigned short			Type;
	static const int size			= 1;

	static inline Type minval()
	{
		return 0;
	}
	static inline Type maxval()
	{
		return USHRT_MAX;
	}

	static const EagleeyeType type = EAGLEEYE_USHORT;
};

template<>
class TypeTrait<int>
{
public:
	typedef int						Type;
	static const int size			= 1;

	static inline Type minval()
	{
		return INT_MIN;
	}
	static inline Type maxval()
	{
		return INT_MAX;
	}

	static const EagleeyeType type = EAGLEEYE_INT;
};

template<>
class TypeTrait<unsigned int>
{
public:
	typedef unsigned int			Type;
	static const int size			= 1;

	static inline Type minval()
	{
		return 0;
	}
	static inline Type maxval()
	{
		return UINT_MAX;
	}

	static const EagleeyeType type = EAGLEEYE_UINT;
};

template<>
class TypeTrait<float>
{
public:
	typedef float					Type;
	static const int size			= 1;

	static inline Type minval()
	{
		return FLT_MIN;
	}
	static inline Type maxval()
	{
		return FLT_MAX;
	}

	static const EagleeyeType type = EAGLEEYE_FLOAT;
};

template<>
class TypeTrait<double>
{
public:
	typedef double					Type;
	static const int size			= 1;

	static inline Type minval()
	{
		return DBL_MIN;
	}
	static inline Type maxval()
	{
		return DBL_MAX;
	}

	static const EagleeyeType type = EAGLEEYE_DOUBLE;
};

template<>
class TypeTrait<RGB>
{
public:
	typedef unsigned char			Type;
	static const int size			= 3;

	static inline RGB minval()
	{
		RGB data;
		data[0] = 0; data[1] = 0; data[2] = 0; data[3] = 0;
		return data;
	}
	static inline RGB maxval()
	{
		RGB data;
		data[0] = 255; data[1] = 255; data[2] = 255; data[3] = 255;
		return data;
	}

	static const EagleeyeType type = EAGLEEYE_RGB;
};

template<>
class TypeTrait<RGBA>
{
public:
	typedef unsigned char			Type;
	static const int size			= 4;

	static inline RGBA minval()
	{
		RGBA data;
		data[0] = 0; data[1] = 0; data[2] = 0; data[3] = 0;
		return data;
	}
	static inline RGBA maxval()
	{
		RGBA data;
		data[0] = 255; data[1] = 255; data[2] = 255; data[3] = 255;
		return data;
	}

	static const EagleeyeType type = EAGLEEYE_RGBA;
};

template<>
class TypeTrait<Array<float,4>>
{
public:
	typedef float			        Type;
	static const int size			= 4;

	static inline Array<float,4> minval()
	{
		Array<float,4> data;
		data[0] = 0; data[1] = 0; data[2] = 0; data[3] = 0;
		return data;
	}
	static inline Array<float,4> maxval()
	{
		Array<float,4> data;
		data[0] = 1.0f; data[1] = 1.0f; data[2] = 1.0f; data[3] = 1.0f;
		return data;
	}

	static const EagleeyeType type = EAGLEEYE_FLOAT4;
};


template<typename T>
class OperateTrait
{
public:
    typedef typename T::ElemType Type;

    static inline Type& unit(T& val,int index = 0)
    {
        return val[index];
    }
    static inline const Type& unit(const T& val,int index = 0)
    {
        return val[index];
    }

    static inline Type maxunit(const T& val)
    {
//        Type max_atomic_val = typename NumericTraits<T>::minval();
//        for (int i = 0; i < typename NumericTraits<T>::size; ++i)
//        {
//            if (val[i] > max_atomic_val)
//            {
//                max_atomic_val = val[i];
//            }
//        }
//
//        return max_atomic_val;
        return Type(0);
    }

    static inline Type minunit(const T& val)
    {
//        Type min_atomic_val = typename NumericTraits<T>::maxval();
//        for (int i = 0; i < typename NumericTraits<T>::size; ++i)
//        {
//            if (val[i] < min_atomic_val)
//            {
//                min_atomic_val = val[i];
//            }
//        }
//
//        return min_atomic_val;
        return Type(0);
    }

    static inline Type square(const T& val)
    {
//        T square_val(0);
//        for (int i = 0; i < typename NumericTraits<T>::size; ++i)
//        {
//            square_val[i] = val[i] * val[i];
//        }
//        return square_val;
        return Type(0);
    }
};

template<>
class OperateTrait<char>
{
public:
    typedef char                            Type;

    static inline Type& unit(char& val,int index = 0)
    {
        return val;
    }
    static inline const Type& unit(const char& val,int index = 0)
    {
        return val;
    }

    static inline Type maxunit(const char& val)
    {
        return val;
    }
    static inline Type minunit(const char& val)
    {
        return val;
    }
    static inline Type square(const char& val)
    {
        return Type(val * val);
    }

};

template<>
class OperateTrait<unsigned char>
{
public:
    typedef unsigned char                        Type;

    static inline Type& unit(unsigned char& val,int index = 0)
    {
        return val;
    }
    static inline const Type& unit(const unsigned char& val,int index = 0)
    {
        return val;
    }

    static inline Type maxunit(const unsigned char& val)
    {
        return val;
    }
    static inline Type minunit(const unsigned char& val)
    {
        return val;
    }
    static inline Type square(const unsigned char& val)
    {
        return Type(val * val);
    }
};

template<>
class OperateTrait<short>
{
public:
    typedef short                                Type;

    static inline Type& unit(short& val,int index = 0)
    {
        return val;
    }
    static inline const Type& unit(const short& val,int index = 0)
    {
        return val;
    }

    static inline Type maxunit(const short& val)
    {
        return val;
    }
    static inline Type minunit(const short& val)
    {
        return val;
    }
    static inline Type square(const short& val)
    {
        return Type(val * val);
    }
};

template<>
class OperateTrait<unsigned short>
{
public:
    typedef unsigned short                        Type;

    static inline Type& unit(unsigned short& val,int index = 0)
    {
        return val;
    }
    static inline const Type& unit(const unsigned short& val,int index = 0)
    {
        return val;
    }

    static inline Type maxunit(const unsigned short& val)
    {
        return val;
    }
    static inline Type minunit(const unsigned short& val)
    {
        return val;
    }
    static inline Type square(const unsigned short& val)
    {
        return Type(val * val);
    }
};

template<>
class OperateTrait<int>
{
public:
    typedef int                                    Type;

    static inline Type& unit(int& val,int index = 0)
    {
        return val;
    }
    static inline const Type& unit(const int& val,int index = 0)
    {
        return val;
    }

    static inline Type maxunit(const int& val)
    {
        return val;
    }
    static inline Type minunit(const int& val)
    {
        return val;
    }
    static inline Type square(const int& val)
    {
        return Type(val * val);
    }
};

template<>
class OperateTrait<unsigned int>
{
public:
    typedef unsigned int                        Type;

    static inline Type& unit(unsigned int& val,int index = 0)
    {
        return val;
    }
    static inline const Type& unit(const unsigned int& val,int index = 0)
    {
        return val;
    }

    static inline Type maxunit(const unsigned int& val)
    {
        return val;
    }
    static inline Type minunit(const unsigned int& val)
    {
        return val;
    }
    static inline Type square(const unsigned int& val)
    {
        return Type(val * val);
    }
};

template<>
class OperateTrait<float>
{
public:
    typedef float                                Type;

    static inline Type& unit(float& val,int index = 0)
    {
        return val;
    }
    static inline const Type& unit(const float& val,int index = 0)
    {
        return val;
    }

    static inline Type maxunit(const float& val)
    {
        return val;
    }
    static inline Type minunit(const float& val)
    {
        return val;
    }
    static inline Type square(const float& val)
    {
        return Type(val * val);
    }
};

template<>
class OperateTrait<double>
{
public:
    typedef double                                Type;

    static inline Type& unit(double& val,int index = 0)
    {
        return val;
    }
    static inline const Type& unit(const double& val,int index = 0)
    {
        return val;
    }

    static inline Type maxunit(const double& val)
    {
        return val;
    }
    static inline Type minunit(const double& val)
    {
        return val;
    }
    static inline Type square(const double& val)
    {
        return Type(val * val);
    }
};

}
#endif
