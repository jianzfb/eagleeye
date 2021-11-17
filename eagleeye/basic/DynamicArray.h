#ifndef _DYNAMICARRAY_H_
#define _DYNAMICARRAY_H_
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/type.h"
#include <memory>

namespace eagleeye
{
/**
 *	@brief dynamic array. The array size could be set dynamically.
 *	@note we should notice the difference between array and vector
 */
template<typename T>
class DynamicArray
{
public:
	typedef T									ElemType;
	/**
	 *	@brief construct functions
	 */
	DynamicArray();
	DynamicArray(int size,T val=T(0));
	template<typename ArrayT,int D>
	DynamicArray(const Array<ArrayT,D>& arr)
	{		
		//judge whether type is compatible
		assert(sizeof(T)==sizeof(ArrayT));
		
		m_size = D;
		m_data = std::shared_ptr<T>(new T[D], [](T* arr) {delete[] arr;});
		memcpy(m_data.get(), &arr, sizeof(T)*D);
	}

	/**
	 *	@brief some cute members
	 */
	int size() const{return m_size;}
	T* dataptr(){return m_data.get();}
	const T* dataptr() const{return m_data.get();}

	/**
	 *	@brief copy the data outside
	 */
	void copy(T* data){
		memcpy(m_data.get(),data,sizeof(T)*m_size);
	}
	
	// /**
	//  *	@brief augment this array
	//  */
	// void aug(const DynamicArray& arr)
	// {
	// 	int size=m_size+arr.size();
	// 	DynamicArray aug_arr(size);
	// 	memcpy(aug_arr.dataptr(),dataptr(),sizeof(T)*m_size);
	// 	memcpy(aug_arr.dataptr()+m_size,arr.dataptr(),sizeof(T)*arr.size());
		
	// 	(*this)=aug_arr;
	// }

	// /**
	//  *	@brief Reshape array to Matrix 
	//  */
	// template<typename MatrixType>
	// inline MatrixType reshape(int r,int c) const
	// {
	// 	typedef typename TypeTrait<T>::AtomicType									ArrayAtomicType;
	// 	typedef typename TypeTrait<typename MatrixType::ElemType>::AtomicType		MatrixAtomicType;

	// 	//judge whether type is compatible
	// 	assert(sizeof(ArrayAtomicType)==sizeof(MatrixAtomicType));

	// 	//judge whether size is the same
	// 	int matrix_total_atomic_nums=r*c*sizeof(typename MatrixType::ElemType)/sizeof(MatrixAtomicType);
	// 	int array_total_atomic_nums=m_size*sizeof(T)/sizeof(ArrayAtomicType);
	// 	assert(matrix_total_atomic_nums==array_total_atomic_nums);

	// 	//
	// 	MatrixType ma(r,c);
	// 	memcpy(ma.dataptr(),dataptr(),sizeof(T)*m_size);

	// 	return ma;
	// }

	T& operator()(int index);
	const T& operator()(int index) const;

	T& operator[](int index);
	const T& operator[](int index) const;
	
	/**
	 *	@brief overload all basic math operations
	 */
	DynamicArray& operator+(const DynamicArray& arr);
	DynamicArray& operator-(const DynamicArray& arr);
	DynamicArray& operator*(const DynamicArray& arr);
	DynamicArray& operator/(const DynamicArray& arr);

	template<typename ScalarT>
	DynamicArray& operator+(ScalarT v){
		T* ptr = this->m_data.get();
		for (int i=0; i<m_size; ++i){
			ptr[i]=ptr[i]+(T)v;
		}

		return *this;
	}

	template<typename ScalarT>
	DynamicArray& operator-(ScalarT v){
		T* ptr = this->m_data.get();
		for (int i=0; i<m_size; ++i){
			ptr[i]=ptr[i]-(T)v;
		}

		return *this;
	}

	template<typename ScalarT>
	DynamicArray& operator*(ScalarT v){
		T* ptr = this->m_data.get();
		for (int i=0;i<m_size;++i){
			ptr[i]=ptr[i]*(T)v;
		}

		return *this;
	}

	template<typename ScalarT>
	DynamicArray& operator/(ScalarT v){
		T* ptr = this->m_data.get();
		for (int i=0;i<m_size;++i){
			ptr[i]=ptr[i]/(T)v;
		}

		return *this;
	}

private:
	std::shared_ptr<T> m_data;
	int m_size;
};

template<typename T>
DynamicArray<T>::DynamicArray()
	:m_size(0){
}
template<typename T>
DynamicArray<T>::DynamicArray(int size,T val)
{
	m_data = std::shared_ptr<T>(new T[size], [](T* arr) {delete[] arr;});
	T* ptr = this->m_data.get();
	m_size=size;
	for (int i=0;i<size;++i){
		ptr[i]=val;
	}
}

template<typename T>
T& DynamicArray<T>::operator()(int index){
	return m_data.get()[index];
}

template<typename T>
const T& DynamicArray<T>::operator()(int index) const{
	return m_data.get()[index];
}

template<typename T>
T& DynamicArray<T>::operator[](int index){
	return m_data.get()[index];
}

template<typename T>
const T& DynamicArray<T>::operator[](int index) const{
	return m_data.get()[index];
}

template<typename T>
DynamicArray<T>& DynamicArray<T>::operator+(const DynamicArray<T>& arr){
	assert(m_size==arr.m_size);
	T* ptr = this->m_data.get();

	for (int i=0;i<m_size;++i){
		ptr[i]=ptr[i]+arr[i];
	}

	return *this;
}

template<typename T>
DynamicArray<T>& DynamicArray<T>::operator-(const DynamicArray<T>& arr){
	assert(m_size==arr.m_size);
	T* ptr = this->m_data.get();

	for (int i=0;i<m_size;++i){
		ptr[i]=ptr[i]-arr[i];
	}

	return *this;
}

template<typename T>
DynamicArray<T>& DynamicArray<T>::operator*(const DynamicArray<T>& arr){
	assert(m_size==arr.m_size);
	T* ptr = this->m_data.get();

	for (int i=0;i<m_size;++i){
		ptr[i]=ptr[i]*arr[i];
	}

	return *this;
}

template<typename T>
DynamicArray<T>& DynamicArray<T>::operator/(const DynamicArray<T>& arr){
	assert(m_size==arr.m_size);
	T* ptr = this->m_data.get();
	for (int i=0;i<m_size;++i){
		ptr[i]=ptr[i]/arr[i];
	}

	return *this;
}

//overloaded output operator for Array
template<typename T>
std::ostream& operator<<(std::ostream& os,const DynamicArray<T>& ob){
	int size=ob.size();
	for (int i=0;i<size;++i){
		os<<ob[i]<<'\t';
	}
	return os;
}

template<typename T>
std::istream& operator>>(std::istream& in,DynamicArray<T>& ob){
	int size=ob.size();
	for (int i=0;i<size;++i){
		in>>ob[i];
	}

	//if input fails
	if (!in){
		ob=DynamicArray<T>();
	}
	return in;
}	

typedef DynamicArray<float>				ArrayFd;
typedef DynamicArray<int>				ArrayId;
typedef DynamicArray<double>			ArrayDd;
}
#endif
