#ifndef _METAOPERATION_H_
#define _METAOPERATION_H_

#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/type.h"
#include <math.h>
#include "eagleeye/basic/Matrix.h"

namespace eagleeye
{
template<typename MetaSrcT,typename MetaTargetT>
class NormalizeMINMAX
{
public:
	typedef MetaSrcT							MetaSrcType;
	typedef MetaTargetT							MetaTargetType;
	
	NormalizeMINMAX(MetaTargetT target_min_val=MetaTargetType(0),MetaTargetT target_max_val=MetaTargetType(1)){
		target_min_meta_val = float(target_min_val);
		target_max_meta_val = float(target_max_val);
		save_time_target_var = target_max_meta_val - target_min_meta_val;		
	};

	void init(const Matrix<MetaSrcT>& data){
		// get min max value
		int rows = data.rows();
		int cols = data.cols();
		src_min_meta_val = 1000000000.0f;
		src_max_meta_val = -1000000000.0f;
		for(int i=0; i<rows; ++i){
			const MetaSrcType* data_ptr = data.row(i);
			for(int j=0; j<cols; ++j){
				if(data_ptr[j]<src_min_meta_val){
					src_min_meta_val = data_ptr[j];
				}
				if(data_ptr[j]>src_max_meta_val){
					src_max_meta_val = data_ptr[j];
				}
			}
		}
		save_time_src_var = src_max_meta_val - src_min_meta_val;
	}

	inline void operator()(const MetaSrcT& meta_val,MetaTargetType& target_meta_val)
	{
		if ( float(meta_val) < src_min_meta_val )
		{
			target_meta_val = MetaTargetT(target_min_meta_val);
			return;
		}
		else if ( float(meta_val) > src_max_meta_val )
		{
			target_meta_val = MetaTargetT(target_max_meta_val);
			return;
		}

		target_meta_val = MetaTargetT(((float(meta_val) - src_min_meta_val) / (save_time_src_var+eagleeye_eps)) * save_time_target_var + target_min_meta_val);
		return;
	}

private:
	float src_min_meta_val;
	float src_max_meta_val;
	float save_time_src_var;

	float target_min_meta_val;
	float target_max_meta_val;
	float save_time_target_var;
};


template<typename MetaSrcT, typename MetaTargetT>
class ThresholdOperation
{
public:
	typedef MetaSrcT							MetaSrcType;
	typedef MetaTargetT							MetaTargetType;

	ThresholdOperation(MetaSrcT threadshold = MetaSrcT( 0 )):meta_threshold(threadshold){};
	
	inline void operator()(const MetaSrcT& meta_val,MetaTargetT& target_meta_val)
	{
		if ( meta_val > meta_threshold )
		{
			target_meta_val = MetaTargetT( 1 );
			return;
		}
		else
		{	
			target_meta_val = MetaTargetT( 0 );
			return;
		}	
	}

	void init(const Matrix<MetaSrcT>& data){
		// do nothing
	}

	MetaSrcT meta_threshold;
};


template<typename MetaSrcT,typename MetaTargetT,int s_one,int s_another>
class SwitchOperations
{
public:
	typedef MetaSrcT							MetaSrcType;
	typedef MetaTargetT							MetaTargetType;

	typedef typename MetaSrcT::ElemType			MetaSrcElemType;
	typedef typename MetaTargetT::ElemType		MetaTargetElemType;

	SwitchOperations(){};

	inline void operator ()(const MetaSrcT& src_meta_val, MetaTargetT& target_meta_val)
	{
		target_meta_val = src_meta_val;
		memcpy(&target_meta_val,&src_meta_val,EAGLEEYE_MIN(sizeof(MetaSrcT),sizeof(MetaTargetT)));

		MetaSrcElemType temp = src_meta_val[s_one];
		target_meta_val[s_one] = src_meta_val[s_another];
		target_meta_val[s_another] = (MetaTargetElemType)temp;
	}

	void init(const Matrix<MetaSrcT>& data){
		// do nothing
	}

};

template<typename MetaSrcT,typename MetaTargetT>
class AverageOperations
{
public:
	typedef MetaSrcT							MetaSrcType;
	typedef MetaTargetT							MetaTargetType;

	inline void operator()(const MetaSrcT& src_meta_val,MetaTargetT& target_meta_val)
	{
		target_meta_val = 0;
		for (int i = 0; i < TypeTrait<MetaSrcType>::size; ++i)
		{
			target_meta_val += MetaTargetType(OperateTrait<MetaSrcType>::unit(src_meta_val,i));
		}

		target_meta_val = target_meta_val / TypeTrait<MetaSrcType>::size;
	}

	void init(const Matrix<MetaSrcT>& data){
		// do nothing
	}

};


template<typename MetaSrcT>
class RGB2LabOp{
public:
	typedef MetaSrcT							MetaSrcType;
	typedef Array<float,3> 						MetaTargetT;
	typedef MetaTargetT							MetaTargetType;

	RGB2LabOp(){

	}
	virtual ~RGB2LabOp(){
	}

	inline void operator()(const MetaSrcT& rgb_pixel, Array<float,3>& lab_pixel){
		float var_R = rgb_pixel[0]/255.0f;
		float var_G = rgb_pixel[1]/255.0f;
		float var_B = rgb_pixel[2]/255.0f;

		if ( var_R > 0.04045f ) var_R = pow( (( var_R + 0.055f ) / 1.055f ), 2.4f );
		else                   var_R = var_R / 12.92f;
		if ( var_G > 0.04045f ) var_G = pow( ( ( var_G + 0.055f ) / 1.055f ), 2.4f);
		else                   var_G = var_G / 12.92f;
		if ( var_B > 0.04045f ) var_B = pow( ( ( var_B + 0.055f ) / 1.055f ), 2.4f);
		else                   var_B = var_B / 12.92f;

		var_R = var_R * 100.f;
		var_G = var_G * 100.f;
		var_B = var_B * 100.f;

		//Observer. = 2°, Illuminant = D65
		float X = var_R * 0.4124f + var_G * 0.3576f + var_B * 0.1805f;
		float Y = var_R * 0.2126f + var_G * 0.7152f + var_B * 0.0722f;
		float Z = var_R * 0.0193f + var_G * 0.1192f + var_B * 0.9505f;


		float var_X = X / 95.047f ;         //ref_X =  95.047   Observer= 2°, Illuminant= D65
		float var_Y = Y / 100.000f;          //ref_Y = 100.000
		float var_Z = Z / 108.883f;          //ref_Z = 108.883

		if ( var_X > 0.008856f ) var_X = pow(var_X , ( 1.f/3.f ) );
		else                    var_X = ( 7.787f * var_X ) + ( 16.f / 116.f );
		if ( var_Y > 0.008856f ) var_Y = pow(var_Y , ( 1.f/3.f ));
		else                    var_Y = ( 7.787f * var_Y ) + ( 16.f / 116.f );
		if ( var_Z > 0.008856f ) var_Z = pow(var_Z , ( 1.f/3.f ));
		else                    var_Z = ( 7.787f * var_Z ) + ( 16.f / 116.f );

		lab_pixel[0] = ( 116.f * var_Y ) - 16.f;
		lab_pixel[1] = 500.f * ( var_X - var_Y );
		lab_pixel[2] = 200.f * ( var_Y - var_Z );
	}

	void init(const Matrix<MetaSrcT>& data){
		// do nothing
	}
};

template<typename MetaTargetT>
class Lab2RGBOp{
public:
	typedef Array<float,3> 						MetaSrcT;
	typedef MetaSrcT							MetaSrcType;
	typedef MetaTargetT							MetaTargetType;

	Lab2RGBOp(){
	}
	virtual ~Lab2RGBOp(){
	}

	inline void operator()(const MetaSrcT& lab_pixel, MetaTargetT& rgb_pixel){
		float l_s = lab_pixel[0];
		float a_s = lab_pixel[1];
		float b_s = lab_pixel[2];
		float var_Y = ( l_s + 16.f ) / 116.f;
		float var_X = a_s / 500.f + var_Y;
		float var_Z = var_Y - b_s / 200.f;

		if ( pow(var_Y,3) > 0.008856f ) 
			var_Y = pow(var_Y,3);
		else                      
			var_Y = ( var_Y - 16.f / 116.f ) / 7.787f;
		if ( pow(var_X,3) > 0.008856f ) 
			var_X = pow(var_X,3);
		else                      
			var_X = ( var_X - 16.f / 116.f ) / 7.787f;
		if ( pow(var_Z,3) > 0.008856f ) 
			var_Z = pow(var_Z,3);
		else                      
			var_Z = ( var_Z - 16.f / 116.f ) / 7.787f;

		float X = 95.047f * var_X ;    	//ref_X =  95.047     Observer= 2°, Illuminant= D65
		float Y = 100.000f * var_Y  ;   	//ref_Y = 100.000
		float Z = 108.883f * var_Z ;    	//ref_Z = 108.883

		var_X = X / 100.f ;       	//X from 0 to  95.047      (Observer = 2°, Illuminant = D65)
		var_Y = Y / 100.f ;       	//Y from 0 to 100.000
		var_Z = Z / 100.f ;      	//Z from 0 to 108.883

		float var_R = var_X *  3.2406f + var_Y * -1.5372f + var_Z * -0.4986f;
		float var_G = var_X * -0.9689f + var_Y *  1.8758f + var_Z *  0.0415f;
		float var_B = var_X *  0.0557f + var_Y * -0.2040f + var_Z *  1.0570f;

		if ( var_R > 0.0031308f ) 
			var_R = 1.055f * pow(var_R , ( 1 / 2.4f ))  - 0.055f;
		else                     
			var_R = 12.92f * var_R;
		if ( var_G > 0.0031308f ) 
			var_G = 1.055f * pow(var_G , ( 1 / 2.4f ) )  - 0.055f;
		else                     
			var_G = 12.92f * var_G;
		if ( var_B > 0.0031308f ) 
			var_B = 1.055f * pow( var_B , ( 1 / 2.4f ) ) - 0.055f;
		else                     
			var_B = 12.92f * var_B;

		rgb_pixel[0] = var_R * 255.f;
		rgb_pixel[1] = var_G * 255.f;
		rgb_pixel[2] = var_B * 255.f;
	}

	void init(const Matrix<MetaSrcT>& data){
		// do nothing
	}
};

template<typename MetaSrcT>
class RGB2HSV{
public:	
	typedef MetaSrcT							MetaSrcType;
	typedef Array<float,3> 						MetaTargetT;
	typedef MetaTargetT							MetaTargetType;

	RGB2HSV(){}
	virtual ~RGB2HSV(){}

	inline void operator()(const MetaSrcT& rgb_pixel, Array<float,3>& hsv_pixel){
		float fpixel[3];
		fpixel[0] = rgb_pixel[0] / 255.0f;
		fpixel[1] = rgb_pixel[1] / 255.0f;
		fpixel[2] = rgb_pixel[2] / 255.0f;

		float min, max, delta;
		// rgb -> 0,1,2
		min = fpixel[0] < fpixel[1] ? fpixel[0] : fpixel[1];
		min = min  < fpixel[2] ? min  : fpixel[2];

		max = fpixel[0] > fpixel[1] ? fpixel[0] : fpixel[1];
		max = max  > fpixel[2] ? max  : fpixel[2];

		hsv_pixel[2] = max;                                // v
		delta = max - min;
		if (delta < 0.00001)
		{
			hsv_pixel[1] = 0;
			hsv_pixel[0] = 0; // undefined, maybe nan?
			return;
		}
		if( max > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
			hsv_pixel[1] = (delta / max);                  // s
		} else {
			// if max is 0, then r = g = b = 0              
			// s = 0, h is undefined
			hsv_pixel[1] = 0.0;
			hsv_pixel[0] = NAN;                            // its now undefined
			return;
		}
		if( fpixel[0] >= max )                           // > is bogus, just keeps compilor happy
			hsv_pixel[0] = ( fpixel[1] - fpixel[2] ) / delta;        // between yellow & magenta
		else
		if( fpixel[1] >= max )
			hsv_pixel[0] = 2.0 + ( fpixel[2] - fpixel[0] ) / delta;  // between cyan & yellow
		else
			hsv_pixel[0] = 4.0 + ( fpixel[0] - fpixel[1] ) / delta;  // between magenta & cyan

		hsv_pixel[0] *= 60.0;                              // degrees

		if( hsv_pixel[0] < 0.0 )
			hsv_pixel[0] += 360.0;
	}

	void init(const Matrix<MetaSrcT>& data){
		// do nothing
	}
};


template<typename MetaTargetT>
class HSV2RGB{
public:
	typedef Array<float,3> 						MetaSrcT;
	typedef MetaSrcT							MetaSrcType;
	typedef MetaTargetT							MetaTargetType;

	HSV2RGB(){};
	virtual ~HSV2RGB(){};

	inline void operator()(const MetaSrcT& hsv_pixel, MetaTargetT& rgb_pixel){
		float      hh, p, q, t, ff;
		long        i;

		if(hsv_pixel[1] <= 0.0) {       // < is bogus, just shuts up warnings
			rgb_pixel[0] = hsv_pixel[2] * 255;
			rgb_pixel[1] = hsv_pixel[2] * 255;
			rgb_pixel[2] = hsv_pixel[2] * 255;
			return;
		}
		hh = hsv_pixel[0];
		if(hh >= 360.0) hh = 0.0;
		hh /= 60.0;
		i = (long)hh;
		ff = hh - i;
		p = hsv_pixel[2] * (1.0 - hsv_pixel[1]);
		q = hsv_pixel[2] * (1.0 - (hsv_pixel[1] * ff));
		t = hsv_pixel[2] * (1.0 - (hsv_pixel[1] * (1.0 - ff)));

		switch(i) {
		case 0:
			rgb_pixel[0] = hsv_pixel[2] * 255;
			rgb_pixel[1] = t * 255;
			rgb_pixel[2] = p * 255;
			break;
		case 1:
			rgb_pixel[0] = q * 255;
			rgb_pixel[1] = hsv_pixel[2] * 255;
			rgb_pixel[2] = p * 255;
			break;
		case 2:
			rgb_pixel[0] = p * 255;
			rgb_pixel[1] = hsv_pixel[2] * 255;
			rgb_pixel[2] = t * 255;
			break;

		case 3:
			rgb_pixel[0] = p * 255;
			rgb_pixel[1] = q * 255;
			rgb_pixel[2] = hsv_pixel[2] * 255;
			break;
		case 4:
			rgb_pixel[0] = t * 255;
			rgb_pixel[1] = p * 255;
			rgb_pixel[2] = hsv_pixel[2] * 255;
			break;
		case 5:
		default:
			rgb_pixel[0] = hsv_pixel[2] * 255;
			rgb_pixel[1] = p * 255;
			rgb_pixel[2] = q * 255;
			break;
		} 
	}
	void init(const Matrix<MetaSrcT>& data){
		// do nothing
	}
};

template<typename MetaTargetT>
class RGB2GRAY{
public:
	typedef Array<unsigned char,3> 						MetaSrcT;
	typedef MetaSrcT									MetaSrcType;
	typedef MetaTargetT									MetaTargetType;

	RGB2GRAY(){
	}
	virtual ~RGB2GRAY(){
	}

	inline void operator()(const MetaSrcT& rgb_pixel, MetaTargetT& gray_pixel){
		// Y=0.21∗R+0.72∗G+0.07∗B
		gray_pixel = 0.21f * rgb_pixel[0] + 0.72f * rgb_pixel[1] + 0.07f * rgb_pixel[2];
	}

	void init(const Matrix<MetaSrcT>& data){
		// do nothing
	}
};

//////////////////////////////////////////////////////////////////////////
template<class ValT>
class GreaterThan
{
public:
	GreaterThan(ValT val = ValT(0)):threshold_val(val){};
	~GreaterThan(){};

	inline bool operator()(const ValT& cur_val)
	{
		if (cur_val > threshold_val)
			return true;
		else
			return false;
	}

private:
	ValT threshold_val;
};

template<class ValT>
class LessThan
{
public:
	LessThan(ValT val = ValT(0)):threshold_val(val){};
	~LessThan(){};

	inline bool operator()(const ValT& cur_val)
	{
		if (cur_val > threshold_val)
			return true;
		else
			return false;
	}

private:
	ValT threshold_val;
};

}
#endif
