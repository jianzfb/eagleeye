#include "eagleeye/processnode/SerializeNode.h"
#include "eagleeye/framework/pipeline/StringSignal.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/common/CJsonObject.hpp"

namespace eagleeye{
SerializeNode::SerializeNode(){
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new StringSignal(), 0);
}

SerializeNode::~SerializeNode(){
}

void SerializeNode::executeNodeInfo(){
    neb::CJsonObject json_obj;
    for(int sig_i=0; sig_i<this->getNumberOfInputSignals(); ++sig_i){
        AnySignal* input_sig = this->getInputPort(sig_i);
        if(input_sig->getSignalCategory() == SIGNAL_CATEGORY_STRING){
            std::string key = std::to_string(sig_i);
            StringSignal* str_sig = (StringSignal*)input_sig;
            std::string value = str_sig->getData();
            json_obj.Add(key, value);
        }
        else if(input_sig->getSignalCategory() == SIGNAL_CATEGORY_IMAGE){
            if(input_sig->getValueType() == EAGLEEYE_FLOAT){
                ImageSignal<float>* data_sig = (ImageSignal<float>*)input_sig;
                Matrix<float> data = data_sig->getData();
                int rows = data.rows();
                int cols = data.cols();
                neb::CJsonObject data_obj;
                for(int i=0; i<rows; ++i){
                    for(int j=0; j<cols; ++j){
                        data_obj.Add(data.at(i,j));
                    }
                }
                neb::CJsonObject matrix_obj;
                matrix_obj.Add("data", data_obj);
                matrix_obj.Add("rows", rows);
                matrix_obj.Add("cols", cols);
                
                std::string key = std::to_string(sig_i);
                json_obj.Add(key, matrix_obj);
            }
            else if(input_sig->getValueType() == EAGLEEYE_INT){
                ImageSignal<int>* data_sig = (ImageSignal<int>*)input_sig;
                Matrix<int> data = data_sig->getData();
                int rows = data.rows();
                int cols = data.cols();
                neb::CJsonObject data_obj;
                for(int i=0; i<rows; ++i){
                    for(int j=0; j<cols; ++j){
                        data_obj.Add(data.at(i,j));
                    }
                }
                neb::CJsonObject matrix_obj;
                matrix_obj.Add("data", data_obj);
                matrix_obj.Add("rows", rows);
                matrix_obj.Add("cols", cols);
                
                std::string key = std::to_string(sig_i);
                json_obj.Add(key, matrix_obj);
            }
        }
        else if(input_sig->getSignalCategory() == SIGNAL_CATEGORY_TENSOR){
            TensorSignal* data_sig = (TensorSignal*)input_sig;
            Tensor data = data_sig->getData();
            if(data.type() == EAGLEEYE_FLOAT){
                float* data_ptr = data.cpu<float>();
                int size = data.dims().production();
                neb::CJsonObject data_obj;
                for(int i=0; i<size; ++i){
                    data_obj.Add(data_ptr[i]);
                }

                neb::CJsonObject shape_obj;
                for(int i=0; i<data.dims().size(); ++i){
                    shape_obj.Add(int(data.dims()[i]));
                }

                neb::CJsonObject tensor_obj;
                tensor_obj.Add("data", data_obj);
                tensor_obj.Add("dim_num", int(data.dims().size()));
                tensor_obj.Add("dims", shape_obj);

                std::string key = std::to_string(sig_i);
                json_obj.Add(key, tensor_obj);
            }
            else if(data.type() == EAGLEEYE_INT){
                int* data_ptr = data.cpu<int>();
                int size = data.dims().production();
                neb::CJsonObject data_obj;
                for(int i=0; i<size; ++i){
                    data_obj.Add(data_ptr[i]);
                }

                neb::CJsonObject shape_obj;
                for(int i=0; i<data.dims().size(); ++i){
                    shape_obj.Add(int(data.dims()[i]));
                }

                neb::CJsonObject tensor_obj;
                tensor_obj.Add("data", data_obj);
                tensor_obj.Add("dim_num", int(data.dims().size()));
                tensor_obj.Add("dims", shape_obj);

                std::string key = std::to_string(sig_i);
                json_obj.Add(key, tensor_obj);                
            }
        }
    }

    std::string str = json_obj.ToFormattedString();
    StringSignal* str_sig = (StringSignal*)this->getOutputPort(0);
    str_sig->setData(str);
}
}