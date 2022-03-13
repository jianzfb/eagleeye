namespace eagleeye{
TensorRTRun::TensorRTRun(std::string model_name, 
            std::string device,
            std::vector<std::string> input_names,
            std::vector<std::vector<int64_t>> input_shapes,
            std::vector<std::string> output_names,
            std::vector<std::vector<int64_t>> output_shapes,
            int omp_num_threads, 
            int cpu_affinity_policy, 
            std::string writable_path){
}

TensorRTRun::~TensorRTRun(){

}

bool TensorRTRun::run(std::map<std::string, unsigned char*> inputs, 
					 std::map<std::string, unsigned char*>& outputs){
    
    // build engine
    // create context for thi sengine

    // allocate buffers for input and output

}

bool TensorRTRun::initialize(){
    return true;
}

void TensorRTRun::setWritablePath(std::string writable_path){

}

std::string TensorRTRun::getWritablePath(){
    return "";
}
} // namespace eagleeye
