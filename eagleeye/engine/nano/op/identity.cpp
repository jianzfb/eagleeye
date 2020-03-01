#include "eagleeye/engine/nano/op/identity.h"
#include "eagleeye/common/EagleeyeLog.h"
namespace eagleeye
{
Identity::Identity(int input_data_num, int output_data_num, std::string op_name):
             FixedCNNOp(input_data_num, output_data_num, FIXED_IDENTITY, op_name){
}   
Identity::~Identity(){

}
void Identity::run_on_cpu(std::vector<Tensor<float>>& output){
    std::cout<<"as input on cpu"<<std::endl;
}

void Identity::run_on_gpu(std::vector<Tensor<float>>& output){
    std::cout<<"as input on gpu"<<std::endl;    
}

void Identity::run_on_cpu(std::vector<Tensor<float>>& output, std::vector<Tensor<float>>& input){
    // EAGLEEYE_LOGD("run on cpu in indentity");
    assert(output[0].getRuntime().type() == EAGLEEYE_CPU);
    // EAGLEEYE_LOGD("check finish");
    // std::cout<<"input size "<<input.size()<<" output size "<<output.size()<<std::endl;

    void* input_data = input[0].cpu();
    void* output_data = output[0].cpu();

    std::cout<<"sleep 5 senconds"<<std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void Identity::run_on_gpu(std::vector<Tensor<float>>& output, std::vector<Tensor<float>>& input){
    EAGLEEYE_LOGD("run on gpu in indentity");
    assert(output[0].getRuntime().type() == EAGLEEYE_GPU);
    EAGLEEYE_LOGD("check finish");

    void* input_data = input[0].gpu();
    void* output_data = output[0].gpu();

    std::cout<<"sleep 1 senconds"<<std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
}


} // namespace eagleeye
