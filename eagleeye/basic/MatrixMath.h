#ifndef _MATRIXMATH_H_
#define _MATRIXMATH_H_
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/type.h"
#include <vector>
#include <math.h>
#include <vector>
#include <algorithm>
#include <limits>
#include <assert.h>
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/basic/ApproxMath.h"
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
#include "eagleeye/common/EagleeyeOpenCL.h"
#endif

namespace eagleeye{
enum InterpMethod
{
	LINEAR_INTERPOLATION,
	BILINEAR_INTERPOLATION,
	SPLINE_INTERPOLATION,
	NEAREST_NEIGHBOR_INTERPOLATION
};

/**	
 *	@brief get the sort index
 *	@note make sure data is a vector
 */
template<class CompareT>
std::vector<unsigned int> sort(const Matrix<typename CompareT::ElementType>& data);

/**
 *	@brief This struct would be used in the sort algorithm
 */
template<class T>
struct SortElement
{
	SortElement(){};
	SortElement(T v,unsigned int i):value(v),index(i){};
	T value;
	unsigned int index;
};

/**
 *	@brief This struct is used in the sort function
 *	@note we have to define operator < for T type
 */
template<typename T>
struct AscendingSort
{
	typedef		T		ElementType;
	bool operator()(const SortElement<T>& a,const SortElement<T>& b)
	{
		return a.value < b.value;	
	}
};

/**
 *	@brief This struct is used in the sort function
 *	@note we have to define operator > for T type
 */
template<typename T>
struct DescendingSort
{
	typedef  T			ElementType;
	bool operator()(const SortElement<T>& a,const SortElement<T>& b)
	{
		return a.value > b.value;
	}
};


template<typename T>
struct gt{
	typedef  T			ElementType;
	bool operator()(const ElementType& a,const ElementType& b)
	{
		return a >= b;
	}
};

template<typename T>
struct ggt{
	typedef  T			ElementType;
	bool operator()(const ElementType& a,const ElementType& b)
	{
		return a > b;
	}
};

template<typename T>
struct lt{
	typedef  T			ElementType;
	bool operator()(const ElementType& a,const ElementType& b)
	{
		return a <= b;
	}
};

template<typename T>
struct llt{
	typedef  T			ElementType;
	bool operator()(const ElementType& a,const ElementType& b)
	{
		return a < b;
	}
};

template<typename T>
struct eq{
	typedef  T			ElementType;
	bool operator()(const ElementType& a,const ElementType& b)
	{
		return a == b;
	}
};

template<typename CompareT>
Matrix<int> where(const Matrix<typename CompareT::ElementType>& x, typename CompareT::ElementType t);

template<typename CompareT, typename OutputT>
Matrix<OutputT> boolean(const Matrix<typename CompareT::ElementType>& x, typename CompareT::ElementType t);

template<typename T>
Matrix<T> msqrt(const Matrix<T>& x){
	unsigned int rows = x.rows();
	unsigned int cols = x.cols();

	Matrix<T> s(rows,cols);
	for(unsigned int r=0; r < rows; ++r){
		const T* x_ptr = x.row(r);
		T* s_ptr = s.row(r);

		for(unsigned int c=0; c<cols; ++c){
			s_ptr[c] = sqrt(x_ptr[c]);
		}
	}

	return s;
}

template<typename T>
Matrix<T> msquare(const Matrix<T>& x){
	unsigned int rows = x.rows();
	unsigned int cols = x.cols();

	Matrix<T> s(rows,cols);
	for(unsigned int r=0; r < rows; ++r){
		const T* x_ptr = x.row(r);
		T* s_ptr = s.row(r);

		for(unsigned int c=0; c<cols; ++c){
			s_ptr[c] = pow(x_ptr[c], 2.0);
		}
	}

	return s;
}

template<typename T>
Matrix<T> msum(const Matrix<T>& x, int axis=1){
	unsigned int rows = x.rows();
	unsigned int cols = x.cols();

	if(axis == 0){
		Matrix<T> s(1,cols);
		T* s_ptr = s.row(0);

		for(unsigned int r=0; r < rows; ++r){
			const T* x_ptr = x.row(r);
			for(unsigned int c=0; c<cols; ++c){
				s_ptr[c] += x_ptr[c];
			}
		}

		return s;
	}
	else{
		Matrix<T> s(rows,1);
		T* s_ptr = s.dataptr();

		for(unsigned int r=0; r < rows; ++r){
			const T* x_ptr = x.row(r);
			for(unsigned int c=0; c<cols; ++c){
				s_ptr[r] += x_ptr[c];
			}
		}

		return s;
	}

}

template<typename T>
float vsum(const Matrix<T>& data){
	unsigned int rows = data.rows();
	unsigned int cols = data.cols();

	float ret = 0;
	for(unsigned int r = 0; r < rows; ++r){
		const T* ptr = data.row(r);
		for (unsigned int c = 0; c < cols; ++c) {
            ret += ptr[c];
		}
	}

	return ret;
}

template<typename T>
Matrix<T> mmean(const Matrix<T>& x, int axis=1){
	unsigned int rows = x.rows();
	unsigned int cols = x.cols();

	if(axis == 0){
		Matrix<T> s(1,cols);
		T* s_ptr = s.row(0);

		for(unsigned int r=0; r < rows; ++r){
			const T* x_ptr = x.row(r);
			for(unsigned int c=0; c<cols; ++c){
				s_ptr[c] += x_ptr[c];
			}
		}
		for(unsigned int c=0; c<cols; ++c){
			s_ptr[c] = s_ptr[c]/float(rows);
		}

		return s;
	}
	else{
		Matrix<T> s(rows,1);
		T* s_ptr = s.dataptr();

		for(unsigned int r=0; r < rows; ++r){
			const T* x_ptr = x.row(r);
			for(unsigned int c=0; c<cols; ++c){
				s_ptr[r] += x_ptr[c];
			}
		}
		for(unsigned int r=0; r < rows; ++r){
			s_ptr[r] = s_ptr[r] / float(cols);
		}

		return s;
	}

}

template<typename T>
float vmean(const Matrix<T>& data){
	unsigned int rows = data.rows();
	unsigned int cols = data.cols();

	float ret = 0;
	for(unsigned int r = 0; r < rows; ++r){
		const T* ptr = data.row(r);
		for (unsigned int c = 0; c < cols; ++c) {
            ret += ptr[c];
		}
	}
	ret /= (rows * cols);
	return ret;
}

template<typename T>
Matrix<T> mmin(const Matrix<T>& x, int axis=1){
	unsigned int rows = x.rows();
	unsigned int cols = x.cols();

	if(axis == 0){
		Matrix<T> s(1,cols);
		T* s_ptr = s.row(0);
		for(unsigned int c=0; c<cols; ++c){
			s_ptr[c] = TypeTrait<T>::maxval();
		}

		for(unsigned int r=0; r < rows; ++r){
			const T* x_ptr = x.row(r);
			for(unsigned int c=0; c<cols; ++c){
				if(s_ptr[c] > x_ptr[c]){
					s_ptr[c] = x_ptr[c];
				}
			}
		}

		return s;
	}
	else{
		Matrix<T> s(rows,1);
		T* s_ptr = s.dataptr();
		for(unsigned int r=0; r<rows; ++r){
			s_ptr[r] = TypeTrait<T>::maxval();
		}

		for(unsigned int r=0; r<rows; ++r){
			const T* x_ptr = x.row(r);
			for(unsigned int c=0; c<cols; ++c){
				if(s_ptr[r] > x_ptr[c]){
					s_ptr[r] = x_ptr[c];
				}
			}
		}

		return s;
	}
}

template<typename T>
Matrix<T> mexp(const Matrix<T>& x){
	unsigned int rows = x.rows();
	unsigned int cols = x.cols();

	Matrix<T> s(rows, cols);
	for(unsigned int r=0; r<rows; ++r){
		const T* x_ptr = x.row(r);
		T* s_ptr = s.row(r);
		for(unsigned int c=0; c<cols; ++c){
			s_ptr[c] = exp(x_ptr[c]);
		}
	}
	return s;
}

template<typename T>
Matrix<T> mexp_(Matrix<T>& x){
	unsigned int rows = x.rows();
	unsigned int cols = x.cols();

	for(unsigned int r=0; r<rows; ++r){
		T* x_ptr = x.row(r);
		for(unsigned int c=0; c<cols; ++c){
			x_ptr[c] = exp(x_ptr[c]);
		}
	}
	return x;
}

template<typename T>
Matrix<T> msigmoid(const Matrix<T>& x){
	unsigned int rows = x.rows();
	unsigned int cols = x.cols();

	Matrix<T> s(rows, cols);
	for(unsigned int r=0; r<rows; ++r){
		const T* x_ptr = x.row(r);
		T* s_ptr = s.row(r);
		for(unsigned int c=0; c<cols; ++c){
			s_ptr[c] = 1.0 / (1+exp(-x_ptr[c]));
		}
	}
	return s;
}

template<typename T>
Matrix<T> msoftmax(const Matrix<T>& x){
	unsigned int rows = x.rows();
	unsigned int cols = x.cols();

	T x_max = x.at(0,0);
	for(unsigned int r=0; r<rows; ++r){
		const T* x_ptr = x.row(r);
		for(unsigned int c=0; c<cols; ++c){
			if(x_max < x_ptr[c]){
				x_max = x_ptr[c];
			}
		}
	}

	Matrix<T> s(rows, cols);
	for(unsigned int r=0; r<rows; ++r){
		const T* x_ptr = x.row(r);
		T* s_ptr = s.row(r);
		float d = 0.0f;		
		for(unsigned int c=0; c<cols; ++c){
			s_ptr[c] = expf(x_ptr[c] - x_max);
			d += s_ptr[c];
		}

		for(unsigned int c=0; c<cols; ++c){
			s_ptr[c] = s_ptr[c] / d;
		}
	}

	return s;
}

template<typename T>
Matrix<T> msoftmaxApprox2(const Matrix<T>& x){
	unsigned int rows = x.rows();
	unsigned int cols = x.cols();
	EAGLEEYE_CHECK(cols == 2, "only support cols = 2");

	Matrix<T> s(rows, cols);
	for(unsigned int r=0; r<rows; ++r){
		const T* x_ptr = x.row(r);
		T* s_ptr = s.row(r);
		s_ptr[0] = 1.0f/(1.0f + expapprox(x_ptr[1] - x_ptr[0]));
		s_ptr[1] = 1.0f - s_ptr[0];
	}

	return s;
}

template<typename T>
Matrix<T> msoftmax2(const Matrix<T>& x){
	unsigned int rows = x.rows();
	unsigned int cols = x.cols();
	EAGLEEYE_CHECK(cols == 2, "only support cols = 2");

	Matrix<T> s(rows, cols);
	for(unsigned int r=0; r<rows; ++r){
		const T* x_ptr = x.row(r);
		T* s_ptr = s.row(r);
		s_ptr[0] = 1.0f/(1.0f + expf(x_ptr[1] - x_ptr[0]));
		s_ptr[1] = 1.0f - s_ptr[0];
	}

	return s;
}

template<typename T>
Matrix<T> mmax(const Matrix<T>& b, T a, std::vector<unsigned int> order, int offset=0);

template<typename T>
Matrix<T> mmax_(Matrix<T>& b, T a, std::vector<unsigned int> order, int offset);

template<typename T>
Matrix<T> mmax(const Matrix<T>& b, T a);

template<typename T>
Matrix<T> mmax_(Matrix<T>& b, T a);

template<typename T>
Matrix<T> mmin(const Matrix<T>& b, T a, std::vector<unsigned int> order, int offset=0);

template<typename T>
Matrix<T> mmin_(Matrix<T>& b, T a, std::vector<unsigned int> order, int offset);

template<typename T>
Matrix<T> mmin(const Matrix<T>& b, T a);

template<typename T>
Matrix<T> mmin_(Matrix<T>& b, T a);

template<typename T>
void meshgrid(const Matrix<T>& x, const Matrix<T>& y, Matrix<T>& xg, Matrix<T>& yg);

template<typename T>
Matrix<T> arange(int s,int e, int stride, int axis=1);

template<typename T>
Matrix<T> concat(std::vector<Matrix<T>> xl, unsigned int axis=0){
	assert(axis == 0 || axis == 1);
	unsigned int num = xl.size();
	if(num == 0){
		return Matrix<T>();
	}

	if(axis == 0){
		unsigned int rows = 0;
		for(unsigned int i=0; i<num; ++i){
			rows += xl[i].rows();
		}

		unsigned int cols = xl[0].cols();
		Matrix<T> result(rows, cols);
		unsigned int offset = 0;
		for(unsigned int i=0; i<num; ++i){
			result(Range(offset, offset+xl[i].rows()), Range(0, cols)).copy(xl[i]);
			offset += xl[i].rows();
		}

		return result;
	}
	else{
		unsigned int rows = xl[0].rows();
		unsigned int cols = 0;
		for(unsigned int i=0; i<num; ++i){
			cols += xl[i].cols();
		}
		Matrix<T> result(rows, cols);

		unsigned int offset = 0;
		for(unsigned int i=0; i<num; ++i){
			result(Range(0, rows),Range(offset, offset+xl[i].cols())).copy(xl[i]);
			offset += xl[i].cols();
		}
		return result;
	}
}

template<typename T>
Matrix<T> tile(const Matrix<T>&x, std::vector<int> reps){
	assert(reps.size() > 0);
	int r_reps = 0;
	int c_reps = 0;
	if(reps.size() < 2){
		r_reps = 1;
		c_reps = reps[0];
	}
	else{
		r_reps = reps[0];
		c_reps = reps[1];
	}

	int x_rows = x.rows();
	int x_cols = x.cols();

	int x_tiled_rows = x_rows * r_reps;
	int x_tiled_cols = x_cols * c_reps;

	Matrix<T> tiled_x(x_tiled_rows, x_tiled_cols);
	for(int i=0; i<x_tiled_rows; ++i){
		for(int j=0; j<c_reps; ++j){
			memcpy(tiled_x.row(i)+j*x_cols, x.row(i%x_rows), sizeof(T)*x_cols);
		}
	}
	return tiled_x;
}

Matrix<float> matan2(Matrix<float> y, Matrix<float> x);

template<typename T>
std::vector<Matrix<T>> split(const Matrix<T> data, int num, int axis=0);

template<typename T>
Matrix<T> norm(const Matrix<T>& x, int mode=0);

template<typename T>
Matrix<int> argmax(const Matrix<T>& x, int axis=0);

template<typename T>
Matrix<T> resize(const Matrix<T> img,float scale,InterpMethod interp_method);

template<typename T>
Matrix<T> resize(const Matrix<T> img,int after_r,int after_c,InterpMethod interp_method);

Matrix<Array<unsigned char, 3>> resize(const Matrix<Array<unsigned char, 3>> img,
										int after_r,int after_c,
										InterpMethod interp_method);

Matrix<unsigned char> resize(const Matrix<unsigned char> img,
										int after_r,int after_c,
										InterpMethod interp_method);

Matrix<float> resize(const Matrix<float> img,
							int after_r,int after_c,
							InterpMethod interp_method);

/**
 * @brief resize by fixed output(RGB)
 * 
 * @param img 
 * @param output 
 * @param interp_method 
 */
void resize(const Matrix<Array<unsigned char, 3>> input,
				Matrix<Array<unsigned char, 3>>& output,
				InterpMethod interp_method);

void resize(const Matrix<Array<unsigned char, 3>> input,
				unsigned char* output,
				int output_r,
				int output_c,
				InterpMethod interp_method);

/**
 * @brief resize by fixed output(float)
 * 
 * @param img 
 * @param output 
 * @param interp_method 
 */
void resize(const Matrix<float> input,
				Matrix<float>& output,
				InterpMethod interp_method);

void resize(const Matrix<float> input,
				float* output,
				int output_r,
				int output_c,
				InterpMethod interp_method);

/**
 * @brief compute hanning window
 * 
 * @param size 
 * @return Matrix<float> 
 */
Matrix<float> hanning(int size);

/**
 * @brief compute outer of vector a and b
 * 
 * @param a 
 * @param b 
 * @return Matrix<float> 
 */
Matrix<float> outer(Matrix<float> a, Matrix<float> b);

/**
 * @brief wrap image with optical flow
 * 
 * @param img 
 * @param op_flow 
 * @return Matrix<Array<unsigned char,3>> 
 */
Matrix<Array<unsigned char,3>> warp(Matrix<Array<unsigned char,3>> img, Matrix<Array<float,2>> op_flow);

/**
 * @brief rotation90 
 * 
 * @param img 
 * @return Matrix<Array<unsigned char,3>> 
 */
Matrix<Array<unsigned char,3>> rotation90right(Matrix<Array<unsigned char,3>> img);

/**
 * @brief rotation180 
 * 
 * @param img 
 * @return Matrix<Array<unsigned char,3>> 
 */
Matrix<Array<unsigned char,3>> rotation180right(Matrix<Array<unsigned char,3>> img);

/**
 * @brief rotation270
 * 
 * @param img 
 * @return Matrix<Array<unsigned char,3>> 
 */
Matrix<Array<unsigned char,3>> rotation270right(Matrix<Array<unsigned char,3>> img);
}


#include "MatrixMath.hpp"
#endif