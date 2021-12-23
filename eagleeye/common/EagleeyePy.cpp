#include "eagleeye/common/EagleeyePy.h"
#ifdef SUPPORT_BOOST_PYTHON
namespace eagleeye{    
void reference_contiguous_array(PyObject* in, PyArrayObject*& in_con, void*& ptr, int& count){
    in_con = PyArray_GETCONTIGUOUS((PyArrayObject*)in);
    ptr = (void*)PyArray_DATA(in_con);
    int num_dim = PyArray_NDIM(in_con);
    npy_intp* pdim = PyArray_DIMS(in_con);
    count = 1;
    for (int i = 0; i < num_dim; i++){
        count *= pdim[i];
    }
}
}
#endif