#ifndef _EAGLEEYE_MATRIX_H_
#define _EAGLEEYE_MATRIX_H_
#include <assert.h>
#include <stdio.h>
#include <ostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <stdlib.h>
#include <cstdlib>
#include <numeric>
#include <functional>
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/type.h"
#include "eagleeye/basic/blob.h"

namespace eagleeye
{
template<typename T>
class Matrix:public Blob
{
public:
	typedef T								ElemType;

	/**
	 *	@brief null constructor
	 */
	Matrix();

	/**
	 * @brief create matrix with (rows, cols)
	 */ 
	Matrix(unsigned int rows,
			unsigned int cols, 
			EagleeyeRuntime runtime=EagleeyeRuntime(EAGLEEYE_CPU),
			Aligned aligned=Aligned(64));
	
	/**
	 * @brief create matrix with shape
	 */ 
	Matrix(std::vector<int64_t> shape,
			EagleeyeRuntime runtime=EagleeyeRuntime(EAGLEEYE_CPU),
			Aligned aligned=Aligned(64));

	/**
	 * @brief fill matrix with val
	 */ 
	Matrix(unsigned int rows,
			unsigned int cols,
			T val, 
			EagleeyeRuntime runtime=EagleeyeRuntime(EAGLEEYE_CPU),
			Aligned aligned=Aligned(64));

	/**
	 *	@brief using matrix structure to wrap outside data
	 *	@note if copy_flag == true, it would copy this outside data; 
	 */
	Matrix(unsigned int rows,
			unsigned int cols,
			void* data,
			bool copy_flag = false, 
			EagleeyeRuntime runtime=EagleeyeRuntime(EAGLEEYE_CPU),
			Aligned aligned=Aligned(64));

	/**
	 * @brief create matrix by texture_id placed in GPU
	 * @note ignore T
	 */ 
	Matrix(unsigned int texture_id);

	/**
	 * @brief Destroy the Matrix object
	 * 
	 */
	virtual ~Matrix(){}

	/**
	 *	@brief overload operator =
	 */
	template<class ScalarType>
	inline Matrix& operator=(ScalarType value)
	{
		int me_rows = rows();
		int me_cols = cols();
		for (int i = 0; i < me_rows; ++i)
		{
			T* data = row(i);
			for (int j = 0; j < me_cols; ++j)
			{
				data[j] = (T)value;
			}
		}
		return *this;
	}

	inline Matrix operator()(Range r_range,Range c_range);
	inline const Matrix operator()(Range r_range,Range c_range) const;

	/**
	 *	@brief check whether this matrix is empty
	 */
	inline bool isempty() const;

	/**
	 *	@brief get data at one specific pos(with offset)
	 */
	inline T& at(unsigned int r_index,unsigned int c_index) const;
	inline T& at(unsigned int index) const;

	inline T& operator()(int r_index,int c_index) const;
	inline T& operator()(int index) const;
	inline T& operator[](int index) const;

	/**
	 *	@brief get the row data pointer(with offset)
	 */
	inline T* row(unsigned int r_index);
	inline const T* row(unsigned int r_index) const;
	
	/**
	 *	@brief get the data pointer at any pos(with offset)
	 */
	inline T* anyptr(unsigned int index);
	inline const T* anyptr(unsigned int index) const;

	/**
	 *	@brief transpose matrix
	 *	@detail It would generate a new matrix.
	 */
	inline Matrix t();

	/**
	 *	@brief build a independent data copy
	 *	@note It would use a new reference count
	 */
	inline Matrix<T> clone() const;

	/**
	 *	@brief get the raw data pointer(without offset)
	 */
	inline T* dataptr();
	inline const T* dataptr() const;

	/**
	 *	@brief set zero for all elements
	 */
	inline void setzeros();
	
	inline Matrix<T> sub_(T v);
	inline Matrix<T> add_(T v);
	inline Matrix<T> mul_(T v);
	inline Matrix<T> div_(T v);

	inline Matrix<T> sub_(const Matrix<T>& v);
	inline Matrix<T> add_(const Matrix<T>& v);

	/**
	 *	@brief set val for matrix
	 */
	inline void setval(T val);
	template<class ConditionT>
	inline void setval(T val,ConditionT cond_t)
	{
		int me_rows = rows();
		int me_cols = cols();

		for (int i = 0; i < me_rows; ++i)
		{
			T* me_ptr = row(i);
			for (int j = 0; j < me_cols; ++j)
			{
				if (cond_t(me_ptr[j]))
					me_ptr[j] = val;
			}
		}
	}

	/**
	 *	@brief copy src data(with offset)
	 *	@note copy self size data from src matrix. Guarantee
	 *	self size isn't greater than src one
	 */
	inline void copy(const Matrix& src);

	/**
	 *	@brief transform the current matrix to any target matrix
	 *	@note It forces to change every element type of every element in the current
	 *	matrix
	 */
	template<typename TargetT>
	inline Matrix<TargetT> transform() const
	{
		typedef typename TypeTrait<T>::Type						SrcAtomicType;
		typedef typename TypeTrait<TargetT>::Type				TargetAtomicType;

		int mat_rows = rows();
		int mat_cols = cols();
		Matrix<TargetT> target_mat(mat_rows,mat_cols);

		for (int i = 0; i < mat_rows; ++i)
		{
			TargetT* target_mat_data = target_mat.row(i);
			const T* src_mat_data = row(i);
			for (int j = 0; j < mat_cols; ++j)
			{
				for (int index = 0; index < TypeTrait<TargetT>::size; ++index)
				{
					OperateTrait<TargetT>::unit(target_mat_data[j],index) = 
						TargetAtomicType(OperateTrait<T>::unit(src_mat_data[j],index%TypeTrait<T>::size));
				}
			}
		}

		return target_mat;
	}

	/**
	 * @brief 行列式（只支持2x2 或 3x3）
	 */ 
	T determinant();

	/**
	 * @brief 矩阵逆
	 */ 
	Matrix<T> inv();

	/**
	 * 
	 */ 
	Matrix<T> cofactor();

	/**
	 *	@brief using the user defined operator to transform every element of the current 
	 *	matrix to target matrix
	 *	@note how to design transform operator, please see "MetaOperation.h" \n
	 *	Generally speaking, this kind of operator has to overlap "operator ()(element)" 
	 */
	template<typename OpT>
	inline Matrix<typename OpT::MetaTargetType> transform(OpT opt = OpT(), Matrix<typename OpT::MetaTargetType> B=Matrix<typename OpT::MetaTargetType>()) const
	{
		typedef typename OpT::MetaTargetType								TargetT;

		int mat_rows = rows();
		int mat_cols = cols();
		Matrix<TargetT> target_mat;
		if(!B.isempty()){
			target_mat = B;
		}
		else{
			target_mat = Matrix<TargetT>(mat_rows,mat_cols);
		}
		
		opt.init(*this);
		for (int i = 0; i < mat_rows; ++i)
		{
			TargetT* target_mat_data = target_mat.row(i);
			const T* src_mat_data = row(i);
			for (int j = 0; j < mat_cols; ++j)
			{
				opt(src_mat_data[j],target_mat_data[j]);
			}
		}

		return target_mat;
	}

	/**
	 *	@brief modify the matrix shape
	 *	@note it would create a new matrix
	 */
	inline Matrix reshape(int r,int c) const;
	inline Matrix reshape_(int r, int c);

	/**
	 *	@brief flip matrix from left to right
	 */
	inline void fliplr();

	/**
	 *	@brief flip matrix from up to down
	 */
	inline void flipud();

	/**
	 *	@brief whether the data is continuous?
	 */
	inline bool isContinuous() const;

	/**
	 *	@brief overload operator * .multiply with scalar value or the same type matrix
	 */
	template<class ScalarType>
	inline Matrix operator*(ScalarType value)
	{
		unsigned int t_rows = m_r_range.e - m_r_range.s;
		unsigned int t_cols = m_c_range.e - m_c_range.s;
		
		Matrix mat(t_rows,t_cols);
		for (unsigned int i = 0; i < t_rows; ++i)
		{
			T* r_ptr = row(i);
			T* mat_ptr = mat.row(i);
			for (unsigned int j = 0; j < t_cols; ++j)
			{
				mat_ptr[j] = r_ptr[j] * value;
			}
		}
		return mat;
	}
	inline Matrix operator*(const Matrix& r);

	template<class ScalarType>
	inline Matrix& operator*=(ScalarType value)
	{
		unsigned int t_rows = m_r_range.e - m_r_range.s;
		unsigned int t_cols = m_c_range.e - m_c_range.s;
		
		for (unsigned int i = 0; i < t_rows; ++i)
		{
			T* r_ptr = row(i);
			for (unsigned int j = 0; j < t_cols; ++j)
			{
				r_ptr[j] *= value;
			}
		}

		return *this;
	}

	/**
	 *	@brief overload operator /. divide scalar value
	 */
	template<typename ScalarType>
	inline Matrix operator/(ScalarType value)
	{
		unsigned int rows = m_r_range.e - m_r_range.s;
		unsigned int cols = m_c_range.e - m_c_range.s;

		Matrix result_matrix(rows,cols);

		for (unsigned int i = 0; i < rows; ++i)
		{
			T* data_ptr = row(i);
			T* result_data_ptr = result_matrix.row(i);
			for (unsigned int j = 0; j < cols; ++j)
			{
				result_data_ptr[j] = data_ptr[j] / value;
			}
		}
		
		return result_matrix;
	}

	template<typename ScalarType>
	inline Matrix& operator/=(ScalarType value)
	{
		unsigned int rows = m_r_range.e - m_r_range.s;
		unsigned int cols = m_c_range.e - m_c_range.s;

		for (unsigned int i = 0; i < rows; ++i)
		{
			T* data_ptr = row(i);
			for (unsigned int j = 0; j < cols; ++j)
			{
				data_ptr[j] = data_ptr[j] / value;
			}
		}

		return *this;
	}
	/**
	 *	@brief Matrix plus operation. plus one scalar value or the same type matrix
	 *	@note The current matrix must possess the same size with matrix m
	 */
	template<typename ScalarType>
	inline Matrix operator+(ScalarType value) const
	{
		unsigned int rows = m_r_range.e - m_r_range.s;
		unsigned int cols = m_c_range.e - m_c_range.s;

		Matrix result_matrix(rows,cols);

		for (unsigned int i = 0; i < rows; ++i)
		{
			const T* data_ptr = row(i);
			T* result_data_ptr = result_matrix.row(i);
			for (unsigned int j = 0; j < cols; ++j)
			{
				result_data_ptr[j] = data_ptr[j] + (T)value;
			}
		}

		return result_matrix;
	}
	inline Matrix operator+(const Matrix& m) const;

	template<typename ScalarType>
	inline Matrix& operator+=(ScalarType value)
	{
		unsigned int rows = m_r_range.e - m_r_range.s;
		unsigned int cols = m_c_range.e - m_c_range.s;

		for (unsigned int i = 0; i < rows; ++i)
		{
			T* data_ptr = row(i);
			for (unsigned int j = 0; j < cols; ++j)
			{
				data_ptr[j] = data_ptr[j] + (T)value;
			}
		}

		return *this;
	}
	inline Matrix& operator+=(const Matrix& m);

	/**
	 *	@brief Matrix subtract operation. subtract one scalar value or the same
	 *	type matrix 
	 *	@note The current matrix must possess the same size with matrix m
	 */
	template<typename ScalarType>
	inline Matrix operator-(ScalarType value) const
	{
		unsigned int rows = m_r_range.e - m_r_range.s;
		unsigned int cols = m_c_range.e - m_c_range.s;

		Matrix result_matrix(rows,cols);

		for (unsigned int i = 0; i < rows; ++i)
		{
			const T* data_ptr = row(i);
			T* result_data_ptr = result_matrix.row(i);
			for (unsigned int j = 0; j < cols; ++j)
			{
				result_data_ptr[j] = data_ptr[j] - (T)value;
			}
		}

		return result_matrix;
	}
	inline Matrix operator-(const Matrix& m) const;

	template<typename ScalarType>
	inline Matrix& operator-=(ScalarType value)
	{
		unsigned int rows = m_r_range.e - m_r_range.s;
		unsigned int cols = m_c_range.e - m_c_range.s;

		for (unsigned int i = 0; i < rows; ++i)
		{
			T* data_ptr = row(i);
			for (unsigned int j = 0; j < cols; ++j)
			{
				data_ptr[j] = data_ptr[j] - (T)value;
			}
		}

		return *this;
	}
	inline Matrix& operator-=(const Matrix& m);

	/**
	 *	@brief Matrix mul product
	 *	@note The current matrix must possess the same size with matrix m
	 */
	inline Matrix mul(const Matrix& m) const;
	inline Matrix mul_(const Matrix& m);

	/**
	 * [div description]
	 * @param  m [description]
	 * @return   [description]
	 */
	inline Matrix div(const Matrix& m) const;
	inline Matrix div_(const Matrix& m);

	/**
	 * @brief flatten matrix
	 * 
	 * @return Matrix 
	 */
	inline Matrix flatten() const;

	/**
	 *	@brief get row and col of matrix
	 */
	inline unsigned int rows() const;
	inline unsigned int cols() const;

	/**
	 *	@brief get total element number
	 */
	inline unsigned int size() const;

	/**
	 *	@brief get stride of matrix
	 *	@note sometimes, it's sub-matrix of some matrix
	 */
	inline unsigned int stride() const;

	/**
	 *	@brief get offset row and offset col
	 *	@note sometimes, it's sub-matrix of some matrix
	 */
	inline void offset(unsigned int& offset_r,unsigned int& offset_c) const;

	/**
	 * @brief get memory layout
	 * 
	 * @param int 
	 * @param int 
	 * @param int 
	 * @param int 
	 */
	inline void layout(unsigned int& offset_r, 
						unsigned int& offset_c, 
						unsigned int& h, 
						unsigned int& w) const;

	/**
	 *	@brief check whether matrix is full
	 */
	inline bool isfull() const;

	inline Matrix select(const std::vector<unsigned int>& index, unsigned int offset=0) const;
	inline Matrix gatherND(const Matrix<int>& indices, int group=1) const;
	inline Matrix gather(const Matrix<int>& indices, int axis=0) const;

private:
	inline unsigned int pos(unsigned int r_index,unsigned int c_index) const;
	inline unsigned int pos(unsigned int index) const;
	inline unsigned int pos_r(unsigned int r_index) const;
	inline unsigned int pos_c(unsigned int c_index) const;

	Range m_r_range;
	Range m_c_range;

	unsigned int m_rows;
	unsigned int m_cols;
};

template<typename T>
std::ostream& operator<<(std::ostream& os, const Matrix<T>& ob) 
{
	std::setiosflags(std::ios::fixed);
	int rows = ob.rows();
	int cols = ob.cols();
	for (int i = 0; i < rows; ++i)
	{
		const T* ob_data = ob.row(i);
		for (int j = 0; j < cols; ++j)
		{
			os<<ob_data[j]<<'\t';
		}
		os<<'\n';
	}

	return os;
}

template<typename T>
Matrix<T>& operator<<(Matrix<T>& ob,const T val)
{
	static int serialization_index = 0;
	static int total_elem_num = 0;
	static int ob_p = 0;
	if (ob_p != int(&ob))
	{
		total_elem_num = ob.size();
		serialization_index = 0;
		ob_p = int(&ob);
	}
	
	//get serialization index
	serialization_index = serialization_index % total_elem_num;
	
	//assign value
	ob(serialization_index) = val;
	serialization_index++;

	return ob;
}

template<typename T>
std::istream& operator>>(std::istream& in,Matrix<T>& ob)
{
	int rows = ob.rows();
	int cols = ob.cols();
	for (int i = 0; i < rows; ++i)
	{
		T* ob_data = ob.row(i);
		for (int j = 0; j < cols; ++j)
		{
			in>>ob_data[j];
		}
	}

	if (!in)
	{
		ob = Matrix<T>();
	}

	return in;
}

/**
 *	@brief some useful type
 */
 typedef Matrix<float>						VecF;
 typedef Matrix<double>						VecD;
 typedef Matrix<int>						VecI;
 typedef Matrix<unsigned int>				VecUI;
 typedef Matrix<short>						VecS;
 typedef Matrix<unsigned short>				VecUS;
 typedef Matrix<char>						VecC;
 typedef Matrix<unsigned char>				VecUC;
	
}

#include "Matrix.hpp"
#endif
