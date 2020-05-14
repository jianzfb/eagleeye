#ifndef _EAGLEEYE_TENSOR_H_
#define _EAGLEEYE_TENSOR_H_
#include <assert.h>
#include <stdio.h>
#include <ostream>
#include <iomanip>
#include <vector>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <numeric>
#include <functional>
#include <iostream>
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/MetaOperation.h"
#include "eagleeye/basic/blob.h"
#include "eagleeye/common/EagleeyeRuntime.h"

namespace eagleeye{
typedef	int64_t 	index_t;
template<typename T>
class Tensor:public Blob{
public:
	typedef T 				ElemType;
	Tensor();
	Tensor(std::vector<int64_t> shape, 
			EagleeyeRuntime runtime=EagleeyeRuntime(EAGLEEYE_CPU),
			Aligned aligned=Aligned(64)); 

	Tensor(std::vector<int64_t> shape, 
			void* data, 
			bool copy, 
			EagleeyeRuntime runtime=EagleeyeRuntime(EAGLEEYE_CPU),
			Aligned aligned=Aligned(64)); 
	
	/**
	 * @brief Destroy the Tensor object
	 * 
	 */
	virtual ~Tensor();

	/**
	 * @brief get cpu ptr
	 * 
	 * @return T* 
	 */
	T* dataptr();

	const std::vector<int64_t>& shape();
	Tensor<T> slice(Range x);
	Tensor<T> slice(Range x, Range y);
	Tensor<T> slice(Range x, Range y, Range z);
	Tensor<T> slice(Range x, Range y, Range z, Range m);

	T& at(int64_t x);
	T& at(int64_t x, int64_t y);
	T& at(int64_t x, int64_t y, int64_t z);
	T& at(int64_t x, int64_t y, int64_t z, int64_t m);

	Tensor<T> clone();
	Tensor<T> flatten();
	
	Tensor<T> clear();

	int64_t size();
	int64_t ndim();
	int64_t dim(int index);
	float scale();

	bool isContinue();

protected:
	int64_t offset(int64_t i);
	int64_t offset(const std::vector<int64_t>& v, int64_t i);
	float m_scale;
	DataFormat m_format;
};	
}

#include "Tensor.hpp"
#endif