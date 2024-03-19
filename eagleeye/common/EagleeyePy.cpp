#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <iostream>
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeFile.h"
#include "eagleeye/common/EagleeyeIO.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/GroupSignal.h"
#include "eagleeye/framework/pipeline/StateSignal.h"
#include "eagleeye/framework/pipeline/BooleanSignal.h"
#include "eagleeye/framework/pipeline/LandmarkSignal.h"
#include "eagleeye/framework/pipeline/StringSignal.h"
#include "eagleeye/framework/pipeline/TensorSignal.h"
#include "eagleeye/framework/pipeline/YUVSignal.h"
#include "eagleeye/framework/pipeline/EmptySignal.h"

namespace py = pybind11;
using namespace py::literals;
using namespace eagleeye;
using namespace eagleeye::dataflow;
namespace eagleeye{
std::map<std::string, std::shared_ptr<AnyNode>> node_pools;

void config_node_input(AnyNode*node, int port, py::object input_obj){
    auto array = pybind11::array::ensure(input_obj);
    if (array.dtype() == pybind11::dtype::of<int32_t>()){
        py::buffer_info buf = array.request();
        TensorSignal* tensor_sig = new TensorSignal();

        std::vector<int64_t> shape;
        for(int i=0; i<array.ndim(); ++i){
            shape.push_back(buf.shape[i]);
        }
        tensor_sig->setData(            
            Tensor(shape,
                EAGLEEYE_INT32,
                DataFormat::AUTO,
                buf.ptr)
        );
        // 设置算子输入
        node->setInputPort(tensor_sig, port);
    }
    else if(array.dtype() == pybind11::dtype::of<float>()){
        py::buffer_info buf = array.request();

        TensorSignal* tensor_sig = new TensorSignal();
        std::vector<int64_t> shape;
        for(int i=0; i<array.ndim(); ++i){
            shape.push_back(buf.shape[i]);
        }     
        tensor_sig->setData(            
            Tensor(shape,
                EAGLEEYE_FLOAT32,
                DataFormat::AUTO,
                buf.ptr)
        );
        // 设置算子输入
        node->setInputPort(tensor_sig, port);
    }
    else if(array.dtype() == pybind11::dtype::of<double>()){
        py::buffer_info buf = array.request();

        TensorSignal* tensor_sig = new TensorSignal();
        std::vector<int64_t> shape;            
        for(int i=0; i<array.ndim(); ++i){
            shape.push_back(buf.shape[i]);
        }   
        tensor_sig->setData(            
            Tensor(shape,
                EAGLEEYE_DOUBLE,
                DataFormat::AUTO,
                buf.ptr)
        );          
        // 设置算子输入
        node->setInputPort(tensor_sig, port);
    }
    else if(array.dtype() == pybind11::dtype::of<unsigned char>()){
        py::buffer_info buf = array.request();
        AnySignal* _sig = NULL;
        std::vector<int64_t> shape;
        for(int i=0; i<array.ndim(); ++i){
            shape.push_back(buf.shape[i]);
        }  
        if(shape.size() != 2 && shape.size() != 3){
            EAGLEEYE_LOGE("UCHAR only support DIM=2 or DIM=3");
        }

        if(shape.size() == 3){
            if(shape[2] == 3){
                ImageSignal<Array<unsigned char,3>>* image_sig = new ImageSignal<Array<unsigned char,3>>();
                _sig = image_sig;

                MetaData meta;
                meta.rows = shape[0];
                meta.cols = shape[1];
                meta.needed_rows = shape[0];
                meta.needed_cols = shape[1];
                image_sig->setData(buf.ptr, meta);
            }
            else if(shape[2] == 4){
                ImageSignal<Array<unsigned char,4>>* image_sig= new ImageSignal<Array<unsigned char,4>>();
                _sig = image_sig;

                MetaData meta;
                meta.rows = shape[0];
                meta.cols = shape[1];
                meta.needed_rows = shape[0];
                meta.needed_cols = shape[1];
                image_sig->setData(buf.ptr, meta);
            }
            else{
                EAGLEEYE_LOGE("IMAGE only support channel==3,4");
            }
        }
        else{
            ImageSignal<unsigned char>* image_sig = new ImageSignal<unsigned char>();
            _sig = image_sig;

            MetaData meta;
            meta.rows = shape[0];
            meta.cols = shape[1];
            meta.needed_rows = shape[0];
            meta.needed_cols = shape[1];
            image_sig->setData(buf.ptr, meta);
        }
        // 设置算子输入
        node->setInputPort(_sig, port);
    }
    else{
        EAGLEEYE_LOGE("Fail to config input. Only Support int32/float/double/unsigned char");
    }
}

py::list node_execute(py::str exe_name, py::str node_name, py::str cls_name, py::dict param, py::list input_tensors){
    py::list output_tensors;

    // 创建算子/加载算子
    bool is_new_created = false;
    std::string c_exe_name = py::cast<std::string>(exe_name);
    std::string c_node_name = py::cast<std::string>(node_name);
    std::string c_cls_name = py::cast<std::string>(cls_name);
    if(node_pools.find(c_exe_name+"/"+c_node_name) == node_pools.end()){
        AnyNode* op = CreateNode<>(c_cls_name);
        if(op == NULL){
            return output_tensors;
        }

        is_new_created = true;
        node_pools[c_exe_name+"/"+c_node_name] = std::shared_ptr<AnyNode>(op);
    }
    std::shared_ptr<AnyNode> exe_node = node_pools[c_exe_name+"/"+c_node_name];
    if(is_new_created){
        // TODO, 基于传入的参数，初始化
    }

    // 处理功能节点（IfElseNode, ParallelNode, AsynNode, LogicalNode）
    if(c_cls_name == "IfElseNode"){
        exe_node->config([&](){ 
                // true branch
                // false branch
                AnyNode* true_branch = CreateNode<>("IdentityNode");
                AnyNode* false_branch = CreateNode<>("IdentityNode");

                config_node_input(true_branch, 0, input_tensors[1]);
                config_node_input(false_branch, 0, input_tensors[2]);
                node_pools[c_exe_name+"/"+c_node_name+ "/true"] = std::shared_ptr<AnyNode>(true_branch);
                node_pools[c_exe_name+"/"+c_node_name+ "/false"] = std::shared_ptr<AnyNode>(false_branch);
                return std::vector<AnyNode*>({true_branch, false_branch});
            }
        );

        // 仅保留状态信号
        py::list filter_input_tensors;
        filter_input_tensors.append(input_tensors[0]);
        input_tensors = filter_input_tensors;
    }
    else if(c_cls_name == "ParallelNode"){

    }
    else if(c_cls_name == "AsynNode"){

    }

    /*-----------------------------------------------------------*/
    // 转换py到Pipeline输入
    std::vector<AnySignal*> input_signals;
    for(int input_index=0; input_index < input_tensors.size(); ++input_index){
        auto array = pybind11::array::ensure(input_tensors[input_index]);

		if (!array){
            // 尝试构建BooleanSignal, StringSignal
            if(py::isinstance<py::str>(input_tensors[input_index])){
                //
                std::string input_data = input_tensors[input_index].cast<std::string>();
            }
			return output_tensors;
        }

		if (array.dtype() == pybind11::dtype::of<int32_t>()){
            py::buffer_info buf = array.request();
            TensorSignal* tensor_sig = new TensorSignal();
            input_signals.push_back(tensor_sig);

            std::vector<int64_t> shape;
            for(int i=0; i<array.ndim(); ++i){
                shape.push_back(buf.shape[i]);
            }
            tensor_sig->setData(            
                Tensor(shape,
                    EAGLEEYE_INT32,
                    DataFormat::AUTO,
                    buf.ptr)
            );
            // 设置算子输入
            exe_node->setInputPort(tensor_sig, input_index);
		}
        else if(array.dtype() == pybind11::dtype::of<float>()){
            py::buffer_info buf = array.request();

            TensorSignal* tensor_sig = new TensorSignal();
            input_signals.push_back(tensor_sig);

            std::vector<int64_t> shape;
            for(int i=0; i<array.ndim(); ++i){
                shape.push_back(buf.shape[i]);
            }     
            tensor_sig->setData(            
                Tensor(shape,
                    EAGLEEYE_FLOAT32,
                    DataFormat::AUTO,
                    buf.ptr)
            );
            // 设置算子输入
            exe_node->setInputPort(tensor_sig, input_index);
        }
        else if(array.dtype() == pybind11::dtype::of<double>()){
            py::buffer_info buf = array.request();

            TensorSignal* tensor_sig = new TensorSignal();
            input_signals.push_back(tensor_sig);

            std::vector<int64_t> shape;            
            for(int i=0; i<array.ndim(); ++i){
                shape.push_back(buf.shape[i]);
            }   
            tensor_sig->setData(            
                Tensor(shape,
                    EAGLEEYE_DOUBLE,
                    DataFormat::AUTO,
                    buf.ptr)
            );          
            // 设置算子输入
            exe_node->setInputPort(tensor_sig, input_index);
        }
        else if(array.dtype() == pybind11::dtype::of<unsigned char>()){
            py::buffer_info buf = array.request();
            AnySignal* _sig = NULL;
            std::vector<int64_t> shape;
            for(int i=0; i<array.ndim(); ++i){
                shape.push_back(buf.shape[i]);
            }  
            if(shape.size() != 2 && shape.size() != 3){
                EAGLEEYE_LOGE("UCHAR only support DIM=2 or DIM=3");
                continue;
            }
            if(shape.size() == 3){
                if(shape[2] == 3){
                    ImageSignal<Array<unsigned char,3>>* image_sig = new ImageSignal<Array<unsigned char,3>>();
                    _sig = image_sig;

                    MetaData meta;
                    meta.rows = shape[0];
                    meta.cols = shape[1];
                    meta.needed_rows = shape[0];
                    meta.needed_cols = shape[1];
                    image_sig->setData(buf.ptr, meta);
                }
                else if(shape[2] == 4){
                    ImageSignal<Array<unsigned char,4>>* image_sig= new ImageSignal<Array<unsigned char,4>>();
                    _sig = image_sig;

                    MetaData meta;
                    meta.rows = shape[0];
                    meta.cols = shape[1];
                    meta.needed_rows = shape[0];
                    meta.needed_cols = shape[1];
                    image_sig->setData(buf.ptr, meta);
                }
                else{
                    EAGLEEYE_LOGE("IMAGE only support channel==3,4");
                    continue;
                }
            }
            else{
                ImageSignal<unsigned char>* image_sig = new ImageSignal<unsigned char>();
                _sig = image_sig;

                MetaData meta;
                meta.rows = shape[0];
                meta.cols = shape[1];
                meta.needed_rows = shape[0];
                meta.needed_cols = shape[1];
                image_sig->setData(buf.ptr, meta);
            }
            // 设置算子输入
            exe_node->setInputPort(_sig, input_index);
        }
        else if(array.dtype() == pybind11::dtype::of<bool>()){
            py::buffer_info buf = array.request();
            bool* buf_ptr = (bool*)buf.ptr;
            bool input_data = buf_ptr[0];
            BooleanSignal* condition_sig = new BooleanSignal();
            input_signals.push_back(condition_sig);
            condition_sig->setData(input_data);
            // 设置算子输入
            exe_node->setInputPort(condition_sig, input_index);  
        }
		else {
            EAGLEEYE_LOGE("Dont support input type. Only Support int32/float/double/unsigned char");
			return output_tensors;
		}
    }

    // run
    exe_node->executeNodeInfo();

    // 转换Pipeline输出到py
    int out_signal_num = exe_node->getNumberOfOutputSignals();
    for(int out_i=0; out_i<out_signal_num; ++out_i){
        AnySignal* out_sig = exe_node->getOutputPort(out_i);
        if(out_sig->getSignalCategory() == SIGNAL_CATEGORY_CONTROL){
            BooleanSignal* boolean_out_sig = (BooleanSignal*)out_sig;
            bool boolean_val = boolean_out_sig->getData();
            output_tensors.append(boolean_val);
            continue;   
        }

        void* out_data;         // RESULT DATA POINTER
        size_t* out_data_size;  // RESULT DATA SHAPE (IMAGE HEIGHT, IMAGE WIDTH, IMAGE CHANNEL)
        int out_data_dims=0;    // 3
        int out_data_type=0;    // RESULT DATA TYPE 
        out_sig->getSignalContent(out_data, out_data_size, out_data_dims, out_data_type);
        if(out_data != NULL){
            std::vector<py::ssize_t> shape;
            for(int i=0; i<out_data_dims; ++i){
                shape.push_back(out_data_size[i]);
            }

            if(out_data_type == 4){
                // int
                auto result = py::array_t<int32_t>(py::detail::any_container<ssize_t>(shape), (int32_t*)out_data);
                output_tensors.append(result);
            }
            else if(out_data_type == 6){
                // float
                auto result = py::array_t<float>(py::detail::any_container<ssize_t>(shape), (float*)out_data);
                output_tensors.append(result);
            }
            else if(out_data_type == 7){
                // double
                auto result = py::array_t<double>(py::detail::any_container<ssize_t>(shape), (double*)out_data);
                output_tensors.append(result);
            }
            else if(out_data_type == 1 || out_data_type == 8 || out_data_type == 9){
                // unsigned char
                auto result = py::array_t<unsigned char>(py::detail::any_container<ssize_t>(shape), (unsigned char*)out_data);
                output_tensors.append(result);
            }
            else{
                //
                EAGLEEYE_LOGE("Out data type abnormal (type=%d).", out_data_type);
            }
        }
        else{
            EAGLEEYE_LOGE("Out data abnormal.");
        }        
    }

    for(int input_index=0; input_index<input_signals.size(); ++input_index){
        delete input_signals[input_index];
    }
    return output_tensors;
}

std::map<std::string, std::shared_ptr<Base>> op_pools;
py::list op_execute(py::str exe_name, py::str op_name, py::str cls_name, py::dict param_1, py::dict param_2, py::dict param_3, py::list input_tensors){
    py::list output_tensors;

    // 创建算子/加载算子
    bool is_new_created_op = false;
    std::string c_exe_name = py::cast<std::string>(exe_name);
    std::string c_op_name = py::cast<std::string>(op_name);
    std::string c_cls_name = py::cast<std::string>(cls_name);
    if(op_pools.find(c_exe_name+"/"+c_op_name) == op_pools.end()){
        Base* op = CreateOp<>(c_cls_name);
        if(op == NULL){
            EAGLEEYE_LOGE("No %s op support", c_cls_name.c_str());
            return output_tensors;
        }

        is_new_created_op = true;
        op_pools[c_exe_name+"/"+c_op_name] = std::shared_ptr<Base>(op);
    }
    std::shared_ptr<Base> exe_op = op_pools[c_exe_name+"/"+c_op_name];

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
		if (!array){
            EAGLEEYE_LOGE("Input tensor type abnormal");
			return output_tensors;
        }

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
        else if(array.dtype() == pybind11::dtype::of<bool>()){
            py::buffer_info buf = array.request();

            std::vector<int64_t> shape;
            for(int i=0; i<array.ndim(); ++i){
                shape.push_back(buf.shape[i]);
            }            
            Tensor temp(shape, EAGLEEYE_BOOL, DataFormat::AUTO, buf.ptr);
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
        else if(tensor.type() == EAGLEEYE_BOOL){
            auto result = py::array_t<bool>(py::detail::any_container<ssize_t>(shape), tensor.cpu<bool>());
            output_tensors.append(result);
        }        
    }
    return output_tensors;
}


py::list load_tensor_list(py::str file_path){
    std::string c_file_path = py::cast<std::string>(file_path);
    if(!isfileexist(c_file_path.c_str())){
        EAGLEEYE_LOGE("File %s dont exist", c_file_path.c_str());
        return py::list();
    }

    // 加载tensor list
    EagleeyeIO yard_io;
    yard_io.createReadHandle(c_file_path, READ_BINARY_MODE);

    int64_t tensor_num_array[1];
    void* tensor_num_array_ptr = (void*)tensor_num_array;
    int tensor_num_size = 0;
    yard_io.read(tensor_num_array_ptr, tensor_num_size);
    std::vector<Tensor> tensor_list(tensor_num_array[0]);
    for(int64_t tensor_i=0; tensor_i<tensor_num_array[0]; ++tensor_i){
        // 形状
        int64_t dim_num_array[1];
        void* dim_num_array_ptr = (void*)dim_num_array;
        int dim_num_size = 0;
        yard_io.read(dim_num_array_ptr, dim_num_size);

        // 
        std::vector<int64_t> tensor_shape(dim_num_array[0]);
        void* tensor_shape_ptr = (void*)(tensor_shape.data());
        int tensor_shape_size = 0;
        yard_io.read(tensor_shape_ptr, tensor_shape_size);

        // 类型
        int64_t tensor_type_array[1];
        void* tensor_type_array_ptr = (void*)tensor_type_array;
        int tensor_type_size = 0;
        yard_io.read(tensor_type_array_ptr, tensor_type_size);

        // 数据
        tensor_list[tensor_i] = Tensor(
            tensor_shape,
            EagleeyeType(tensor_type_array[0]),
            DataFormat::AUTO,
            CPU_BUFFER
        );

        int tensor_data_size = 0;
        void* tensor_ptr = tensor_list[tensor_i].cpu();
        yard_io.read(tensor_ptr, tensor_data_size);
    }
    yard_io.destroyHandle();

    // -> python numpy
    py::list output_tensors;
    for(int i=0; i<tensor_list.size(); ++i){
        Tensor tensor = tensor_list[i];
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
        else if(tensor.type() == EAGLEEYE_BOOL){
            auto result = py::array_t<bool>(py::detail::any_container<ssize_t>(shape), tensor.cpu<bool>());
            output_tensors.append(result);
        }
    }

    return output_tensors;
}

bool save_tensor_list(py::str file_path, py::list tensor_list){
    // 解析成Tensor
    std::vector<Tensor> c_tensor_list;
    for(py::handle t: tensor_list){
        auto array = pybind11::array::ensure(t);
		if (array.dtype() == pybind11::dtype::of<int32_t>()){
            py::buffer_info buf = array.request();

            std::vector<int64_t> shape;
            for(int i=0; i<array.ndim(); ++i){
                shape.push_back(buf.shape[i]);
            }
            Tensor temp(shape, EAGLEEYE_INT32, DataFormat::AUTO, buf.ptr);
            c_tensor_list.push_back(temp);
		}
        else if(array.dtype() == pybind11::dtype::of<float>()){
            py::buffer_info buf = array.request();

            std::vector<int64_t> shape;
            for(int i=0; i<array.ndim(); ++i){
                shape.push_back(buf.shape[i]);
            }            
            Tensor temp(shape, EAGLEEYE_FLOAT32, DataFormat::AUTO, buf.ptr);
            c_tensor_list.push_back(temp);
        }
        else if(array.dtype() == pybind11::dtype::of<double>()){
            py::buffer_info buf = array.request();

            std::vector<int64_t> shape;
            for(int i=0; i<array.ndim(); ++i){
                shape.push_back(buf.shape[i]);
            }            
            Tensor temp(shape, EAGLEEYE_DOUBLE, DataFormat::AUTO, buf.ptr);
            c_tensor_list.push_back(temp);
        }
        else if(array.dtype() == pybind11::dtype::of<unsigned char>()){
            py::buffer_info buf = array.request();

            std::vector<int64_t> shape;
            for(int i=0; i<array.ndim(); ++i){
                shape.push_back(buf.shape[i]);
            }            
            Tensor temp(shape, EAGLEEYE_UCHAR, DataFormat::AUTO, buf.ptr);
            c_tensor_list.push_back(temp);
        }
        else if(array.dtype() == pybind11::dtype::of<bool>()){
            py::buffer_info buf = array.request();

            std::vector<int64_t> shape;
            for(int i=0; i<array.ndim(); ++i){
                shape.push_back(buf.shape[i]);
            }            
            Tensor temp(shape, EAGLEEYE_BOOL, DataFormat::AUTO, buf.ptr);
            c_tensor_list.push_back(temp);
        }
		else {
			EAGLEEYE_LOGE("tensor type not support");
            return false;
		}
    }

    // 保存Tensor
    EagleeyeIO yard_io;
    std::string c_file_path = py::cast<std::string>(file_path);
    yard_io.createWriteHandle(c_file_path.c_str(), false, WRITE_BINARY_MODE);
    int64_t tensor_num = c_tensor_list.size();
    // 数量
    yard_io.write(&tensor_num, sizeof(int64_t));
    for(int64_t tensor_i=0; tensor_i<tensor_num; ++tensor_i){
        Tensor tt = c_tensor_list[tensor_i];

        // 形状
        int64_t dim_num = tt.dims().size();
        yard_io.write(&dim_num, sizeof(int64_t));

        std::vector<int64_t> tensor_shape = tt.dims().data();
        yard_io.write(tensor_shape.data(), tensor_shape.size()*sizeof(int64_t));

        // 类型
        int64_t tensor_type = (int64_t)(tt.type());
        yard_io.write(&tensor_type, sizeof(int64_t));

        // 数据
        yard_io.write(tt.cpu(), tt.blobsize());
    }
    yard_io.destroyHandle();
    return true;
}

py::object load_tensor(py::str file_path){
    std::string c_file_path = py::cast<std::string>(file_path);
    if(!isfileexist(c_file_path.c_str())){
        EAGLEEYE_LOGE("File %s dont exist", c_file_path.c_str());
        return py::object();
    }

    // 加载tensor list
    EagleeyeIO yard_io;
    yard_io.createReadHandle(c_file_path, READ_BINARY_MODE);

    // 形状
    int64_t dim_num_array[1];
    void* dim_num_array_ptr = (void*)dim_num_array;
    int dim_num_size = 0;
    yard_io.read(dim_num_array_ptr, dim_num_size);

    // 
    std::vector<int64_t> tensor_shape(dim_num_array[0]);
    void* tensor_shape_ptr = (void*)(tensor_shape.data());
    int tensor_shape_size = 0;
    yard_io.read(tensor_shape_ptr, tensor_shape_size);

    // 类型
    int64_t tensor_type_array[1];
    void* tensor_type_array_ptr = (void*)tensor_type_array;
    int tensor_type_size = 0;
    yard_io.read(tensor_type_array_ptr, tensor_type_size);

    // 数据
    Tensor tensor(
        tensor_shape,
        EagleeyeType(tensor_type_array[0]),
        DataFormat::AUTO,
        CPU_BUFFER
    );

    int tensor_data_size = 0;
    void* tensor_ptr = tensor.cpu();
    yard_io.read(tensor_ptr, tensor_data_size);

    // ->numpy
    std::vector<py::ssize_t> shape;
    Dim tensor_dim = tensor.dims();
    for(int i=0; i<tensor_dim.size(); ++i){
        shape.push_back(tensor_dim[i]);
    }

    if(tensor.type() == EAGLEEYE_INT32){
        auto result = py::array_t<int32_t>(py::detail::any_container<ssize_t>(shape), tensor.cpu<int32_t>());
        return result;
    }
    else if(tensor.type() == EAGLEEYE_FLOAT32){
        auto result = py::array_t<float>(py::detail::any_container<ssize_t>(shape), tensor.cpu<float>());
        return result;
    }
    else if(tensor.type() == EAGLEEYE_DOUBLE){
        auto result = py::array_t<double>(py::detail::any_container<ssize_t>(shape), tensor.cpu<double>());
        return result;
    }
    else if(tensor.type() == EAGLEEYE_UCHAR){
        auto result = py::array_t<unsigned char>(py::detail::any_container<ssize_t>(shape), tensor.cpu<unsigned char>());
        return result;
    }
    else if(tensor.type() == EAGLEEYE_BOOL){
        auto result = py::array_t<bool>(py::detail::any_container<ssize_t>(shape), tensor.cpu<bool>());
        return result;
    }
    
    return py::object();
}

bool save_tensor(py::str file_path, py::object t){
    // 解析成Tensor
    auto array = pybind11::array::ensure(t);
    Tensor temp;
    if (array.dtype() == pybind11::dtype::of<int32_t>()){
        py::buffer_info buf = array.request();

        std::vector<int64_t> shape;
        for(int i=0; i<array.ndim(); ++i){
            shape.push_back(buf.shape[i]);
        }
        temp = Tensor(shape, EAGLEEYE_INT32, DataFormat::AUTO, buf.ptr);
    }
    else if(array.dtype() == pybind11::dtype::of<float>()){
        py::buffer_info buf = array.request();

        std::vector<int64_t> shape;
        for(int i=0; i<array.ndim(); ++i){
            shape.push_back(buf.shape[i]);
        }            
        temp = Tensor(shape, EAGLEEYE_FLOAT32, DataFormat::AUTO, buf.ptr);
    }
    else if(array.dtype() == pybind11::dtype::of<double>()){
        py::buffer_info buf = array.request();

        std::vector<int64_t> shape;
        for(int i=0; i<array.ndim(); ++i){
            shape.push_back(buf.shape[i]);
        }            
        temp = Tensor(shape, EAGLEEYE_DOUBLE, DataFormat::AUTO, buf.ptr);
    }
    else if(array.dtype() == pybind11::dtype::of<unsigned char>()){
        py::buffer_info buf = array.request();

        std::vector<int64_t> shape;
        for(int i=0; i<array.ndim(); ++i){
            shape.push_back(buf.shape[i]);
        }            
        temp = Tensor(shape, EAGLEEYE_UCHAR, DataFormat::AUTO, buf.ptr);
    }
    else if(array.dtype() == pybind11::dtype::of<bool>()){
        py::buffer_info buf = array.request();

        std::vector<int64_t> shape;
        for(int i=0; i<array.ndim(); ++i){
            shape.push_back(buf.shape[i]);
        }            
        temp = Tensor(shape, EAGLEEYE_BOOL, DataFormat::AUTO, buf.ptr);
    }
    else {
        EAGLEEYE_LOGE("tensor type not support");
        return false;
    }

    // 保存Tensor
    EagleeyeIO yard_io;
    std::string c_file_path = py::cast<std::string>(file_path);
    yard_io.createWriteHandle(c_file_path.c_str(), false, WRITE_BINARY_MODE);
    // 形状
    int64_t dim_num = temp.dims().size();
    yard_io.write(&dim_num, sizeof(int64_t));

    std::vector<int64_t> tensor_shape = temp.dims().data();
    yard_io.write(tensor_shape.data(), tensor_shape.size()*sizeof(int64_t));

    // 类型
    int64_t tensor_type = (int64_t)(temp.type());
    yard_io.write(&tensor_type, sizeof(int64_t));

    // 数据
    yard_io.write(temp.cpu(), temp.blobsize());

    yard_io.destroyHandle();
    return true;
}

py::object load_data(py::str file_path, py::str dtype){
    std::string c_file_path = py::cast<std::string>(file_path);
    if(!isfileexist(c_file_path.c_str())){
        EAGLEEYE_LOGE("File %s dont exist", c_file_path.c_str());
        return py::object();
    }

    std::ifstream i_file_handle;
    i_file_handle.open(c_file_path.c_str(),std::ios::binary);
    int size = 0;
    i_file_handle.read((char*)(&size),sizeof(int));
    
    std::string c_dtype = py::cast<std::string>(dtype);
    if(c_dtype == "float32"){
        // float
        py::array_t<float> info(py::detail::any_container<ssize_t>(std::vector<py::ssize_t>{size/sizeof(float)}));
        i_file_handle.read((char*)(info.request().ptr), sizeof(char)*size);
        i_file_handle.close();

        return info;
    }
    else if(c_dtype == "double"){
        // double
        py::array_t<double> info(py::detail::any_container<ssize_t>(std::vector<py::ssize_t>{size/sizeof(double)}));
        i_file_handle.read((char*)(info.request().ptr), sizeof(char)*size);
        i_file_handle.close();  

        return info;    
    }
    else if(c_dtype == "int32"){
        // int32
        py::array_t<int32_t> info(py::detail::any_container<ssize_t>(std::vector<py::ssize_t>{size/sizeof(int32_t)}));
        i_file_handle.read((char*)(info.request().ptr), sizeof(char)*size);
        i_file_handle.close();

        return info;
    }
    else if(c_dtype == "uint8"){
        // int32
        py::array_t<unsigned char> info(py::detail::any_container<ssize_t>(std::vector<py::ssize_t>{size/sizeof(unsigned char)}));
        i_file_handle.read((char*)(info.request().ptr), sizeof(char)*size);
        i_file_handle.close();

        return info;
    }
    else if(c_dtype == "bool"){
        py::array_t<bool> info(py::detail::any_container<ssize_t>(std::vector<py::ssize_t>{size/sizeof(bool)}));
        i_file_handle.read((char*)(info.request().ptr), sizeof(char)*size);
        i_file_handle.close();

        return info; 
    }  
    else{
        i_file_handle.close();
        return py::object();
    }
	
}

bool save_data(py::str file_path, py::object t){
    // 解析成Tensor
    auto array = pybind11::array::ensure(t);
    char* ptr = NULL;
    int size = 0;
    if (array.dtype() == pybind11::dtype::of<int32_t>()){
        py::buffer_info buf = array.request();
        size = 1;
        for(int i=0; i<array.ndim(); ++i){
            size *= buf.shape[i];
        }
        ptr = (char*)buf.ptr;
        size = size * sizeof(int32_t);
    }
    else if(array.dtype() == pybind11::dtype::of<float>()){
        py::buffer_info buf = array.request();
        size = 1;
        for(int i=0; i<array.ndim(); ++i){
            size *= buf.shape[i];
        }
        ptr = (char*)buf.ptr;
        size = size * sizeof(float);
    }
    else if(array.dtype() == pybind11::dtype::of<double>()){
        py::buffer_info buf = array.request();
        size = 1;
        for(int i=0; i<array.ndim(); ++i){
            size *= buf.shape[i];
        }
        ptr = (char*)buf.ptr;
        size = size * sizeof(double);
    }
    else if(array.dtype() == pybind11::dtype::of<unsigned char>()){
        py::buffer_info buf = array.request();
        size = 1;
        for(int i=0; i<array.ndim(); ++i){
            size *= buf.shape[i];
        }
        ptr = (char*)buf.ptr;
        size = size * sizeof(unsigned char);
    }
    else if(array.dtype() == pybind11::dtype::of<bool>()){
        py::buffer_info buf = array.request();
        size = 1;
        for(int i=0; i<array.ndim(); ++i){
            size *= buf.shape[i];
        }
        ptr = (char*)buf.ptr;
        size = size * sizeof(bool);
    }
    else {
        EAGLEEYE_LOGE("data type not support");
        return false;
    }

    std::string c_file_path = py::cast<std::string>(file_path);
    EagleeyeIO yard_io;
    yard_io.createWriteHandle(c_file_path, false, WRITE_BINARY_MODE);
    yard_io.write(ptr, size);
    yard_io.destroyHandle();
}


PYBIND11_MODULE(eagleeye, m) {
    m.doc() = "pybind11 pipline/node/op/io ext"; // optional module docstring
    m.def("node_execute", &node_execute);
    m.def("op_execute", &op_execute);
    m.def("load_tensor_list", &load_tensor_list);
    m.def("save_tensor_list", &save_tensor_list);
    m.def("load_tensor", &load_tensor);
    m.def("save_tensor", &save_tensor);
    m.def("load_data", &load_data);
    m.def("save_data", &save_data);
}
}
