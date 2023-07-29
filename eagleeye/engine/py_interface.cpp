#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <iostream>
#include "eagleeye/engine/nano/op/dynamiccreater.h"


namespace py = pybind11;
using namespace py::literals;
using namespace eagleeye;
using namespace eagleeye::dataflow;
std::map<std::string, std::shared_ptr<Base>> op_pools;

py::list execute(py::str exe_name, py::str op_name, py::dict param_1, py::dict param_2, py::dict param_3, py::list input_tensors){
    py::list output_tensors;

    // 创建算子/加载算子
    bool is_new_created_op = false;
    if(op_pools.find(py::cast<std::string>(op_name)) == op_pools.end()){
        Base* op = CreateOp<>(py::cast<std::string>(op_name));
        if(op == NULL){
            return output_tensors;
        }

        is_new_created_op = true;
        op_pools[op_name] = std::shared_ptr<Base>(op);
    }
    std::shared_ptr<Base> exe_op = op_pools[op_name];

    if(is_new_created_op){
        // 算子初始化，仅在新创建时调用
        // param_1 std::map<std::string, std::vector<float>>
        std::map<std::string, std::vector<float>> op_param_1;
        for (auto param_1_item : param_1){
            std::string var_name = py::cast<std::string>(param_1_item.first);
            for(auto value: param_1_item.second){
                float float_value = py::cast<float>(value);
                op_param_1[var_name].push_back(float_value);
            }
        }

        // param_2 std::map<std::string, std::vector<std::string>> 
        std::map<std::string, std::vector<std::string>> op_param_2;
        for (auto param_2_item : param_2){
            std::string var_name = py::cast<std::string>(param_2_item.first);
            for(auto value: param_2_item.second){
                std::string str_value = py::cast<std::string>(value);
                op_param_2[var_name].push_back(str_value);
            }
        }

        // param_3 std::map<std::string, std::vector<std::vector<float>>> 
        std::map<std::string, std::vector<std::vector<float>>>  op_param_3;
        for (auto param_3_item : param_3){
            std::string var_name = py::cast<std::string>(param_3_item.first);

            std::vector<std::vector<float>> vv;
            for(auto level_1_value: param_3_item.second){
                std::vector<float> jj;
                for(auto level_2_value: level_1_value){
                    jj.push_back(py::cast<float>(level_2_value));
                }
                vv.push_back(jj);
            }

            op_param_3[var_name] = vv;
        }

        exe_op->init(op_param_1);
        exe_op->init(op_param_2);
        exe_op->init(op_param_3);
    }

    // input tensors
    std::vector<Tensor> inputs;
    for( py::handle t: input_tensors){
        auto array = pybind11::array::ensure(t);
		if (!array)
			return output_tensors;

		if (array.dtype() == pybind11::dtype::of<int32_t>()){
            py::buffer_info buf = array.request();

            std::vector<int64_t> shape;
            for(int i=0; i<array.ndim(); ++i){
                shape.push_back(buf.shape[i]);
            }
            Tensor temp(shape, EAGLEEYE_INT32, DataFormat::AUTO, buf.ptr);
            inputs.push_back(temp);
		}
        else if(array.dtype() == pybind11::dtype::of<float>()){
            py::buffer_info buf = array.request();

            std::vector<int64_t> shape;
            for(int i=0; i<array.ndim(); ++i){
                shape.push_back(buf.shape[i]);
            }            
            Tensor temp(shape, EAGLEEYE_FLOAT32, DataFormat::AUTO, buf.ptr);
            inputs.push_back(temp);
        }
        else if(array.dtype() == pybind11::dtype::of<double>()){
            py::buffer_info buf = array.request();

            std::vector<int64_t> shape;
            for(int i=0; i<array.ndim(); ++i){
                shape.push_back(buf.shape[i]);
            }            
            Tensor temp(shape, EAGLEEYE_DOUBLE, DataFormat::AUTO, buf.ptr);
            inputs.push_back(temp);
        }
        else if(array.dtype() == pybind11::dtype::of<unsigned char>()){
            py::buffer_info buf = array.request();

            std::vector<int64_t> shape;
            for(int i=0; i<array.ndim(); ++i){
                shape.push_back(buf.shape[i]);
            }            
            Tensor temp(shape, EAGLEEYE_UCHAR, DataFormat::AUTO, buf.ptr);
            inputs.push_back(temp);
        }
		else {
			return output_tensors;
		}
    }

    // run
    exe_op->runOnCpu(inputs);

    // 将输出tensor转换为 pybind格式
    for(int i=0; i<exe_op->getOutputNum(); ++i){
        Tensor tensor = exe_op->getOutput(i);
        std::vector<py::ssize_t> shape;
        Dim tensor_dim = tensor.dims();
        for(int i=0; i<tensor_dim.size(); ++i){
            shape.push_back(tensor_dim[i]);
        }

        if(tensor.type() == EAGLEEYE_INT32){
            auto result = py::array_t<int32_t>(py::detail::any_container<ssize_t>(shape), tensor.cpu<int32_t>());
            output_tensors.append(result);
        }
        else if(tensor.type() == EAGLEEYE_FLOAT32){
            auto result = py::array_t<float>(py::detail::any_container<ssize_t>(shape), tensor.cpu<float>());
            output_tensors.append(result);
        }
        else if(tensor.type() == EAGLEEYE_DOUBLE){
            auto result = py::array_t<double>(py::detail::any_container<ssize_t>(shape), tensor.cpu<double>());
            output_tensors.append(result);
        }
        else if(tensor.type() == EAGLEEYE_UCHAR){
            auto result = py::array_t<unsigned char>(py::detail::any_container<ssize_t>(shape), tensor.cpu<unsigned char>());
            output_tensors.append(result);
        }
    }
    return output_tensors;
}


PYBIND11_MODULE(eagleeye, m) {
    m.doc() = "pybind11 op ext"; // optional module docstring
    m.def("execute", &execute);
}