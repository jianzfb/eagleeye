namespace eagleeye{
namespace dataflow{
template<std::size_t IN, std::size_t OUT>
int LambdaOp<IN,OUT>::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

template<std::size_t IN, std::size_t OUT>
int LambdaOp<IN,OUT>::runOnCpu(const std::vector<Tensor>& input){
    return this->m_cpu_process(input, this->m_outputs);
}

template<std::size_t IN, std::size_t OUT>
int LambdaOp<IN,OUT>::runOnGpu(const std::vector<Tensor>& input){
    return this->m_gpu_process(input, this->m_outputs);
}
   
} // namespace dataflow
} // namespace eagleeye
