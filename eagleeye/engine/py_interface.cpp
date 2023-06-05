#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <iostream>

namespace py = pybind11;
using namespace py::literals;
// define func 
int add(int i = 1, int j = 2) {
    return i + j;
}

py::list execute(py::str exe_name, py::str op_name, py::dict param_1, py::dict param_2, py::dict param_3, py::list input_tensors){
    py::list output_tensors;

    // param_1
    std::cout<<"param_1 "<<py::len(param_1);
    int param_1_num = py::len(param_1);
    for (auto param_1_item : param_1){
        std::cout<<"key: "<<py::cast<std::string>(param_1_item.first)<<std::endl;
        for(auto v: param_1_item.second){
            std::string v_str = py::cast<std::string>(v);
            std::cout<<"value "<<v_str<<std::endl;
        }
    }

    for( py::handle t: input_tensors){
        auto buf = pybind11::array::ensure(t);
		if (!buf)
			return output_tensors;

		if (buf.dtype() == pybind11::dtype::of<int32_t>())
		{
			std::cout << "int32 numpy" << std::endl;
		}
        else if(buf.dtype() == pybind11::dtype::of<float>()){
            std::cout<<"float numpy"<<std::endl;
        }
        else if(buf.dtype() == pybind11::dtype::of<double>()){
            std::cout<<"double numpy"<<std::endl;   
        }
        else if(buf.dtype() == pybind11::dtype::of<unsigned char>()){
            std::cout<<"uchar numpy"<<std::endl;   
        }
		else {
			std::cout << "No match" << std::endl;
			return output_tensors;
		}
    }

}

PYBIND11_MODULE(eagleeye, m) {
    m.doc() = "pybind11 example-1 plugin"; // optional module docstring

    m.def("add", &add, "A function which adds two numbers",
        py::arg("i") = 1, py::arg("j") = 2)
    .def("execute", &execute);
}