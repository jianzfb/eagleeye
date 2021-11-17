#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/Array.h"

#ifndef _EAGLEEYE_PY_H_
#define _EAGLEEYE_PY_H_
#ifdef SUPPORT_BOOST_PYTHON
#include <boost/python.hpp>
#include <boost/python/stl_iterator.hpp>
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include "numpy/arrayobject.h"
#include <iostream>

namespace eagleeye{
template<typename T>
class PyArg{
public:
	typedef T T2T;
	static T parse(T t){
		return t;
	}
};

template<>
class PyArg<Matrix<float>>{
public:
	typedef boost::python::object T2T;
	static Matrix<float> parse(T2T obj){
		PyObject* pobj = obj.ptr();
		Matrix<float> result;
		if(!pobj){
			return result;
		}
		Py_buffer pybuf;
	    if(PyObject_GetBuffer(pobj, &pybuf, PyBUF_C_CONTIGUOUS)!=-1){
	        void *buf = pybuf.buf;
	        Py_ssize_t *shape = pybuf.shape;
	        int ndim = pybuf.ndim;
	        if(ndim >= 3 || ndim == 0){
	        	PyBuffer_Release(&pybuf);
	        	return Matrix<float>();
	        }

	        int count = 0;
	        if(ndim == 1){
	        	result = Matrix<float>(1, shape[0], buf, true);
	        }
	        else{
	        	result = Matrix<float>(shape[0], shape[1], buf, true);
	        }
	        PyBuffer_Release(&pybuf);
	    }
		return result;
	}
};

template<>
class PyArg<Matrix<double>>{
public:
	typedef boost::python::object T2T;
	static Matrix<double> parse(T2T obj){
		PyObject* pobj = obj.ptr();
		Matrix<double> result;
		if(!pobj){
			return result;
		}
		Py_buffer pybuf;
	    if(PyObject_GetBuffer(pobj, &pybuf, PyBUF_C_CONTIGUOUS)!=-1){
	        void *buf = pybuf.buf;
	        Py_ssize_t *shape = pybuf.shape;
	        int ndim = pybuf.ndim;
	        if(ndim >= 3 || ndim == 0){
	        	PyBuffer_Release(&pybuf);
	        	return Matrix<double>();
	        }
	        int count = 0;
	        if(ndim == 1){
	        	result = Matrix<double>(1, shape[0], buf, true);
	        }
	        else{
	        	result = Matrix<double>(shape[0], shape[1], buf, true);
	        }
	        PyBuffer_Release(&pybuf);
	    }
		return result;
	}
};

template<>
class PyArg<Matrix<int>>{
public:
	typedef boost::python::object T2T;
	static Matrix<int> parse(T2T obj){
		PyObject* pobj = obj.ptr();
		Matrix<int> result;
		if(!pobj){
			return result;
		}
		Py_buffer pybuf;
	    if(PyObject_GetBuffer(pobj, &pybuf, PyBUF_C_CONTIGUOUS)!=-1){
	        void *buf = pybuf.buf;

	        Py_ssize_t *shape = pybuf.shape;
	        int ndim = pybuf.ndim;
	        if(ndim >= 3 || ndim == 0){
	        	PyBuffer_Release(&pybuf);
	        	return Matrix<int>();
	        }
	        int count = 0;
	        if(ndim == 1){
	        	result = Matrix<int>(1, shape[0], buf, true);
	        }
	        else{
	        	result = Matrix<int>(shape[0], shape[1], buf, true);
	        }
	        PyBuffer_Release(&pybuf);
	    }
		return result;
	}
};

template<>
class PyArg<Matrix<Array<unsigned char, 3>>>{
public:
	typedef boost::python::object T2T;
	static Matrix<Array<unsigned char, 3>> parse(T2T obj){
		PyObject* pobj = obj.ptr();
		Matrix<Array<unsigned char, 3>> result;
		if(!pobj){
			return result;
		}
		Py_buffer pybuf;
	    if(PyObject_GetBuffer(pobj, &pybuf, PyBUF_C_CONTIGUOUS)!=-1){
	        void *buf = pybuf.buf;
	        Py_ssize_t *shape = pybuf.shape;
	        int ndim = pybuf.ndim;
	        if(ndim != 3){
	        	PyBuffer_Release(&pybuf);
	        	return Matrix<Array<unsigned char, 3>>();
	        }
	        if(shape[2] != 3){
	        	PyBuffer_Release(&pybuf);
	        	return Matrix<Array<unsigned char, 3>>();
	        }
        	result = Matrix<Array<unsigned char, 3>>(shape[0], shape[1], buf, true);
	        PyBuffer_Release(&pybuf);
	    }
		return result;
	}
};

template<typename T>
class PyReturn{
public:
	typedef T TR;
	static TR parse(T t){
		return t;
	}
	static T defaultval(){
		return T(0);
	}
};

void reference_contiguous_array(PyObject* in, PyArrayObject*& in_con, void*& ptr, int& count);

template<>
class PyReturn<Matrix<float>>{
public:
	typedef PyObject* TR;
	static TR parse(Matrix<float> t){
		int rows = t.rows();
		int cols = t.cols();
	  	npy_intp _sizes[2];
		if(rows == 0 || cols == 0){
			return NULL;
		}

	  	_sizes[0] = rows;
	  	_sizes[1] = cols;

	    PyObject* out_matrix = PyArray_SimpleNew(2, _sizes, NPY_FLOAT);
	    void* ptr_out;
	    int count;
	    PyArrayObject* output_contigous_array;
	    reference_contiguous_array(out_matrix, output_contigous_array, ptr_out, count);

	    float* ptr_out_f = (float*)ptr_out;
	    for(int i=0; i<rows; ++i){
	    	memcpy(ptr_out_f+i*cols, t.row(i), sizeof(float)*cols);
	    }
	 	
	 	Py_DECREF(output_contigous_array);
	 	return out_matrix;
	}

	static Matrix<float> defaultval(){
		return Matrix<float>();
	}
};

template<>
class PyReturn<Matrix<double>>{
public:
	typedef PyObject* TR;
	static TR parse(Matrix<double> t){
		int rows = t.rows();
		int cols = t.cols();
		if(rows == 0 || cols == 0){
			return NULL;
		}

	  	npy_intp _sizes[2];
	  	_sizes[0] = rows;
	  	_sizes[1] = cols;

	    PyObject* out_matrix = PyArray_SimpleNew(2, _sizes, NPY_DOUBLE);
	    void* ptr_out;
	    int count;
	    PyArrayObject* output_contigous_array;
	    reference_contiguous_array(out_matrix, output_contigous_array, ptr_out, count);

	    double* ptr_out_f = (double*)ptr_out;
	    for(int i=0; i<rows; ++i){
	    	memcpy(ptr_out_f+i*cols, t.row(i), sizeof(double)*cols);
	    }
	 	
	 	Py_DECREF(output_contigous_array);
	 	return out_matrix;
	}

	static Matrix<double> defaultval(){
		return Matrix<double>();
	}
};

template<>
class PyReturn<Matrix<int>>{
public:
	typedef PyObject* TR;
	static TR parse(Matrix<int> t){
		int rows = t.rows();
		int cols = t.cols();
		if(rows == 0 || cols == 0){
			return NULL;
		}

	  	npy_intp _sizes[2];
	  	_sizes[0] = rows;
	  	_sizes[1] = cols;

	    PyObject* out_matrix = PyArray_SimpleNew(2, _sizes, NPY_INT);
	    void* ptr_out;
	    int count;
	    PyArrayObject* output_contigous_array;
	    reference_contiguous_array(out_matrix, output_contigous_array, ptr_out, count);

	    int* ptr_out_f = (int*)ptr_out;
	    for(int i=0; i<rows; ++i){
	    	memcpy(ptr_out_f+i*cols, t.row(i), sizeof(int)*cols);
	    }
	 	
	 	Py_DECREF(output_contigous_array);
	 	return out_matrix;
	}

	static Matrix<int> defaultval(){
		return Matrix<int>();
	}
};

#define EAGLEEYE_PYTHON_FUNC(func, r_type) \
	typename eagleeye::PyReturn<r_type>::TR func##_py (){ \
		r_type result = eagleeye::PyReturn<r_type>::defaultval(); \
		func(result); \
		typename eagleeye::PyReturn<r_type>::TR result_py = eagleeye::PyReturn<r_type>::parse(result); \
		return result_py; \
	} 
#define EAGLEEYE_PYTHON_FUNC_D(func, r_type) \
	typename eagleeye::PyReturn<r_type>::TR func##_py (){ \
		r_type result = eagleeye::PyReturn<r_type>::defaultval(); \
		result = func(); \
		typename eagleeye::PyReturn<r_type>::TR result_py = eagleeye::PyReturn<r_type>::parse(result); \
		return result_py; \
	} 		
#define EAGLEEYE_PYTHON_FUNC_ARG(func, r_type, a_type) \
	typename eagleeye::PyReturn<r_type>::TR func##_py (typename eagleeye::PyArg<a_type>::T2T a_py){ \
		r_type result = eagleeye::PyReturn<r_type>::defaultval(); \
		func(eagleeye::PyArg<a_type>::parse(a_py), result); \
		typename eagleeye::PyReturn<r_type>::TR result_py = eagleeye::PyReturn<r_type>::parse(result); \
		return result_py; \
	} 
#define EAGLEEYE_PYTHON_FUNC_ARGN1(func, r_type, a_type) EAGLEEYE_PYTHON_FUNC_ARG(func, r_type, a_type)
#define EAGLEEYE_PYTHON_FUNC_ARGN2(func, r_type, a_type, b_type) \
	typename eagleeye::PyReturn<r_type>::TR func##_py (typename eagleeye::PyArg<a_type>::T2T a_py, typename eagleeye::PyArg<b_type>::T2T b_py){ \
		r_type result = eagleeye::PyReturn<r_type>::defaultval(); \
		func(eagleeye::PyArg<a_type>::parse(a_py), eagleeye::PyArg<b_type>::parse(b_py), result); \
		typename eagleeye::PyReturn<r_type>::TR result_py = eagleeye::PyReturn<r_type>::parse(result); \
		return result_py; \
	}
#define EAGLEEYE_PYTHON_FUNC_ARGN3(func, r_type, a_type, b_type, c_type) \
	typename eagleeye::PyReturn<r_type>::TR func##_py (typename eagleeye::PyArg<a_type>::T2T a_py, typename eagleeye::PyArg<b_type>::T2T b_py, typename eagleeye::PyArg<c_type>::T2T c_py){ \
		r_type result = eagleeye::PyReturn<r_type>::defaultval(); \
		func(eagleeye::PyArg<a_type>::parse(a_py), eagleeye::PyArg<b_type>::parse(b_py), eagleeye::PyArg<c_type>::parse(c_py),result); \
		typename eagleeye::PyReturn<r_type>::TR result_py = eagleeye::PyReturn<r_type>::parse(result); \
		return result_py; \
	}
#define EAGLEEYE_PYTHON_FUNC_ARGN4(func, r_type, a_type, b_type, c_type, d_type) \
	typename eagleeye::PyReturn<r_type>::TR func##_py (typename eagleeye::PyArg<a_type>::T2T a_py, typename eagleeye::PyArg<b_type>::T2T b_py, typename eagleeye::PyArg<c_type>::T2T c_py, typename eagleeye::PyArg<d_type>::T2T d_py){ \
		r_type result = eagleeye::PyReturn<r_type>::defaultval(); \
		func(eagleeye::PyArg<a_type>::parse(a_py), eagleeye::PyArg<b_type>::parse(b_py), eagleeye::PyArg<c_type>::parse(c_py), eagleeye::PyArg<d_type>::parse(d_py),result); \
		typename eagleeye::PyReturn<r_type>::TR result_py = eagleeye::PyReturn<r_type>::parse(result); \
		return result_py; \
	}
#define EAGLEEYE_PYTHON_FUNC_ARGN5(func, r_type, a_type, b_type, c_type, d_type, e_type) \
	typename eagleeye::PyReturn<r_type>::TR func##_py (typename eagleeye::PyArg<a_type>::T2T a_py, typename eagleeye::PyArg<b_type>::T2T b_py, typename eagleeye::PyArg<c_type>::T2T c_py, typename eagleeye::PyArg<d_type>::T2T d_py, typename eagleeye::PyArg<e_type>::T2T e_py){ \
		r_type result = eagleeye::PyReturn<r_type>::defaultval(); \
		func(eagleeye::PyArg<a_type>::parse(a_py), eagleeye::PyArg<b_type>::parse(b_py), eagleeye::PyArg<c_type>::parse(c_py), eagleeye::PyArg<d_type>::parse(d_py), eagleeye::PyArg<e_type>::parse(e_py),result); \
		typename eagleeye::PyReturn<r_type>::TR result_py = eagleeye::PyReturn<r_type>::parse(result); \
		return result_py; \
	}
#define EAGLEEYE_PYTHON_FUNC_ARGN6(func, r_type, a_type, b_type, c_type, d_type, e_type, f_type) \
	typename eagleeye::PyReturn<r_type>::TR func##_py (typename eagleeye::PyArg<a_type>::T2T a_py, typename eagleeye::PyArg<b_type>::T2T b_py, typename eagleeye::PyArg<c_type>::T2T c_py, typename eagleeye::PyArg<d_type>::T2T d_py, typename eagleeye::PyArg<e_type>::T2T e_py, typename eagleeye::PyArg<f_type>::T2T f_py){ \
		r_type result = eagleeye::PyReturn<r_type>::defaultval(); \
		func(eagleeye::PyArg<a_type>::parse(a_py), eagleeye::PyArg<b_type>::parse(b_py), eagleeye::PyArg<c_type>::parse(c_py), eagleeye::PyArg<d_type>::parse(d_py), eagleeye::PyArg<e_type>::parse(e_py),eagleeye::PyArg<f_type>::parse(f_py),result); \
		typename eagleeye::PyReturn<r_type>::TR result_py = eagleeye::PyReturn<r_type>::parse(result); \
		return result_py; \
	}
#define EAGLEEYE_PYTHON_FUNC_ARGN7(func, r_type, a_type, b_type, c_type, d_type, e_type, f_type,g_type) \
	typename eagleeye::PyReturn<r_type>::TR func##_py (typename eagleeye::PyArg<a_type>::T2T a_py, typename eagleeye::PyArg<b_type>::T2T b_py, typename eagleeye::PyArg<c_type>::T2T c_py, typename eagleeye::PyArg<d_type>::T2T d_py, typename eagleeye::PyArg<e_type>::T2T e_py, typename eagleeye::PyArg<f_type>::T2T f_py, typename eagleeye::PyArg<g_type>::T2T g_py){ \
		r_type result = eagleeye::PyReturn<r_type>::defaultval(); \
		func(eagleeye::PyArg<a_type>::parse(a_py), eagleeye::PyArg<b_type>::parse(b_py), eagleeye::PyArg<c_type>::parse(c_py), eagleeye::PyArg<d_type>::parse(d_py), eagleeye::PyArg<e_type>::parse(e_py),eagleeye::PyArg<f_type>::parse(f_py),eagleeye::PyArg<g_type>::parse(g_py),result); \
		typename eagleeye::PyReturn<r_type>::TR result_py = eagleeye::PyReturn<r_type>::parse(result); \
		return result_py; \
	}
#define EAGLEEYE_PYTHON_FUNC_ARGN8(func, r_type, a_type, b_type, c_type, d_type, e_type, f_type,g_type,h_type) \
	typename eagleeye::PyReturn<r_type>::TR func##_py (typename eagleeye::PyArg<a_type>::T2T a_py, typename eagleeye::PyArg<b_type>::T2T b_py, typename eagleeye::PyArg<c_type>::T2T c_py, typename eagleeye::PyArg<d_type>::T2T d_py, typename eagleeye::PyArg<e_type>::T2T e_py, typename eagleeye::PyArg<f_type>::T2T f_py, typename eagleeye::PyArg<g_type>::T2T g_py, typename eagleeye::PyArg<h_type>::T2T h_py){ \
		r_type result = eagleeye::PyReturn<r_type>::defaultval(); \
		func(eagleeye::PyArg<a_type>::parse(a_py), eagleeye::PyArg<b_type>::parse(b_py), eagleeye::PyArg<c_type>::parse(c_py), eagleeye::PyArg<d_type>::parse(d_py), eagleeye::PyArg<e_type>::parse(e_py),eagleeye::PyArg<f_type>::parse(f_py),eagleeye::PyArg<g_type>::parse(g_py), eagleeye::PyArg<h_type>::parse(h_py),result); \
		typename eagleeye::PyReturn<r_type>::TR result_py = eagleeye::PyReturn<r_type>::parse(result); \
		return result_py; \
	}
#define EAGLEEYE_PYTHON_FUNC_ARGN9(func, r_type, a_type, b_type, c_type, d_type, e_type, f_type,g_type,h_type,i_type) \
	typename eagleeye::PyReturn<r_type>::TR func##_py (typename eagleeye::PyArg<a_type>::T2T a_py, typename eagleeye::PyArg<b_type>::T2T b_py, typename eagleeye::PyArg<c_type>::T2T c_py, typename eagleeye::PyArg<d_type>::T2T d_py, typename eagleeye::PyArg<e_type>::T2T e_py, typename eagleeye::PyArg<f_type>::T2T f_py, typename eagleeye::PyArg<g_type>::T2T g_py, typename eagleeye::PyArg<h_type>::T2T h_py,typename eagleeye::PyArg<i_type>::T2T i_py){ \
		r_type result = eagleeye::PyReturn<r_type>::defaultval(); \
		func(eagleeye::PyArg<a_type>::parse(a_py), eagleeye::PyArg<b_type>::parse(b_py), eagleeye::PyArg<c_type>::parse(c_py), eagleeye::PyArg<d_type>::parse(d_py), eagleeye::PyArg<e_type>::parse(e_py),eagleeye::PyArg<f_type>::parse(f_py),eagleeye::PyArg<g_type>::parse(g_py), eagleeye::PyArg<h_type>::parse(h_py),eagleeye::PyArg<i_type>::parse(i_py),result); \
		typename eagleeye::PyReturn<r_type>::TR result_py = eagleeye::PyReturn<r_type>::parse(result); \
		return result_py; \
	}	
#define EAGLEEYE_PYTHON_FUNC_ARGN10(func, r_type, a_type, b_type, c_type, d_type, e_type, f_type,g_type,h_type,i_type,j_type) \
	typename eagleeye::PyReturn<r_type>::TR func##_py (typename eagleeye::PyArg<a_type>::T2T a_py, typename eagleeye::PyArg<b_type>::T2T b_py, typename eagleeye::PyArg<c_type>::T2T c_py, typename eagleeye::PyArg<d_type>::T2T d_py, typename eagleeye::PyArg<e_type>::T2T e_py, typename eagleeye::PyArg<f_type>::T2T f_py, typename eagleeye::PyArg<g_type>::T2T g_py, typename eagleeye::PyArg<h_type>::T2T h_py,typename eagleeye::PyArg<i_type>::T2T i_py,typename eagleeye::PyArg<j_type>::T2T j_py){ \
		r_type result = eagleeye::PyReturn<r_type>::defaultval(); \
		func(eagleeye::PyArg<a_type>::parse(a_py), eagleeye::PyArg<b_type>::parse(b_py), eagleeye::PyArg<c_type>::parse(c_py), eagleeye::PyArg<d_type>::parse(d_py), eagleeye::PyArg<e_type>::parse(e_py),eagleeye::PyArg<f_type>::parse(f_py),eagleeye::PyArg<g_type>::parse(g_py), eagleeye::PyArg<h_type>::parse(h_py),eagleeye::PyArg<i_type>::parse(i_py),eagleeye::PyArg<j_type>::parse(j_py),result); \
		typename eagleeye::PyReturn<r_type>::TR result_py = eagleeye::PyReturn<r_type>::parse(result); \
		return result_py; \
	}	
}
static int EAGLEEYE_PYTHON_INIT(){
    Py_Initialize();
    import_array();
}

#define EAGLEEYE_PYTHON_MODULE(lib) BOOST_PYTHON_MODULE(lib)
namespace py = boost::python;
#define EAGLEEYE_PYTHON_DEF_POINTER(name, func) \
	py::def(#name, func##_py, py::return_value_policy<py::return_opaque_pointer>()); 
#define EAGLEEYE_PYTHON_DEF(name, func) \
	py::def(#name, func##_py); 

template <class T>
inline boost::python::list std_vector_to_py_list(std::vector<T> vector) {
    typename std::vector<T>::iterator iter;
    boost::python::list list;
    for (iter = vector.begin(); iter != vector.end(); ++iter) {
        list.append(*iter);
    }
    return list;
}

#else
#define EAGLEEYE_PYTHON_FUNC_ARGN(func, r_type)
#define EAGLEEYE_PYTHON_FUNC_ARGN1(func, r_type, a_type)
#define EAGLEEYE_PYTHON_FUNC_ARGN2(func, r_type, a_type, b_type)
#define EAGLEEYE_PYTHON_FUNC_ARGN3(func, r_type, a_type, b_type, c_type)
#define EAGLEEYE_PYTHON_FUNC_ARGN4(func, r_type, a_type, b_type, c_type, d_type)
#endif
#endif