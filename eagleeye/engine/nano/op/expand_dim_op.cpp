#include "eagleeye/engine/nano/op/expand_dim_op.h"
namespace eagleeye{
namespace dataflow{
ExpandDimOp::ExpandDimOp(){
    this->m_axis = 0;
}

ExpandDimOp::~ExpandDimOp(){

}

int ExpandDimOp::init(std::map<std::string, std::vector<float>> params){
    if(params.find("axis") != params.end()){
        this->m_axis = (int)(params["axis"][0]);
    }
    return 0;
}

int ExpandDimOp::init(std::map<std::string, std::vector<std::string>> params){
    return 0;
}

int ExpandDimOp::runOnCpu(const std::vector<Tensor>& input){
    Dim input_dim = input[0].dims();
    std::vector<int64_t> out_shape(input_dim.size()+1);
    int count = 0;
    for(int i=0; i<out_shape.size(); ++i){
        if(i == m_axis){
            out_shape[i] = 1;
        }
        else{
            out_shape[i] = input_dim[count];
            count += 1;
        }
    }

    unsigned char* data = const_cast<unsigned char*>(input[0].cpu<unsigned char>());
    this->m_outputs[0] = Tensor(
        out_shape,
        input[0].type(),
        DataFormat::AUTO,
        data
    );
    return 0;
}

int ExpandDimOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}
}    
}