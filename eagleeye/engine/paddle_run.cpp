#ifdef EAGLEEYE_PADDLE_SUPPORT
#include "eagleeye/engine/paddle_run.h"
#include "paddle_api.h"  // NOLINT
#include "eagleeye/common/EagleeyeLog.h"
#include <iostream>
namespace eagleeye{
ModelRun::ModelRun(std::string model_name, 
			   std::string device,
			   std::vector<std::string> input_names,
			   std::vector<std::vector<int64_t>> input_shapes,
			   std::vector<std::string> output_names,
			   std::vector<std::vector<int64_t>> output_shapes,
			   int omp_num_threads, 
			   int cpu_affinity_policy, 
			   std::string writable_path)
	:ModelEngine(model_name,
				 device,
				 input_names,
				 input_shapes,
				 output_names,
				 output_shapes,
				 omp_num_threads,
				 cpu_affinity_policy,
				 writable_path){    
	// paddle model file path
	this->m_nb_path = model_name;
	if(!endswith(model_name, ".nb")){
		this->m_nb_path = this->m_nb_path+".nb";
	}

	if(writable_path.length() == 0){
		if(!ispath(this->m_nb_path)){
			this->m_nb_path = this->getModelRoot() + std::string("/")+this->m_nb_path;
		}
	}
	else{
		if(endswith(writable_path, "/")){
			this->m_nb_path = writable_path +this->m_nb_path;
		}
		else{
			this->m_nb_path = writable_path + std::string("/")+this->m_nb_path;
		}
	}
	EAGLEEYE_LOGD("paddle model %s", this->m_nb_path.c_str());
}    


ModelRun::~ModelRun(){
}
bool ModelRun::run(std::map<std::string, unsigned char*> inputs, 
				   std::map<std::string, unsigned char*>& outputs){	
    // ignore outside inputs
    if(!this->m_predictor.get()){
        EAGLEEYE_LOGE("padle predictor null");
        return false;
    }

    // run model
    this->m_predictor->Run();

    // get output from net
    for(int index = 0; index < this->m_output_names.size(); ++index){
        std::unique_ptr<const paddle::lite_api::Tensor> s = this->m_predictor->GetOutput(index);
        outputs[this->m_output_names[index]] = (unsigned char*)(s->mutable_data<float>());
    }
    return true;
}

bool ModelRun::initialize(){
    // 1. Set MobileConfig
	std::cout<<"A"<<std::endl;
    EAGLEEYE_LOGD("set paddle model file");
    paddle::lite_api::MobileConfig config;
    config.set_model_from_file(this->m_nb_path);
	std::cout<<"B"<<std::endl;

	// if(this->m_device == "GPU"){
	// 	std::cout<<"D"<<std::endl;
	// 	bool is_opencl_backend_valid = paddle::lite_api::IsOpenCLBackendValid();
	// 	std::cout<<"E"<<std::endl;
	// 	if (is_opencl_backend_valid) {
	// 		std::cout<<"F"<<std::endl;
	// 		// give opencl nb model dir
	// 		config.set_opencl_tune(false); // default is false
	// 		std::cout<<"N"<<std::endl;
	// 	} else {
	// 		std::cout<<"G"<<std::endl;
	// 		EAGLEEYE_LOGE("Unsupport opencl nb model.");
	// 		exit(1);
	// 		// you can give backup cpu nb model instead
	// 		// config.set_model_from_file(cpu_nb_model_dir);
	// 	}
	// }

	std::cout<<"R"<<std::endl;
    // NOTE: To load model transformed by model_optimize_tool before
    // release/v2.3.0, plese use `set_model_dir` API as listed below.
    // config.set_model_dir(model_dir);
    config.set_power_mode(paddle::lite_api::LITE_POWER_HIGH);
    config.set_threads(this->m_omp_num_threads);
	std::cout<<"C"<<std::endl;

    // 2. Create PaddlePredictor by MobileConfig
    EAGLEEYE_LOGD("create paddle predictor");
    m_predictor = paddle::lite_api::CreatePaddlePredictor<paddle::lite_api::MobileConfig>(config);
    if(!m_predictor.get()){
        return false;
    }
	std::cout<<"D"<<std::endl;
    // 3.step resize tensor
    EAGLEEYE_LOGD("initialize tensor");
    for(int index = 0; index < this->m_input_names.size(); ++index){
        std::unique_ptr<paddle::lite_api::Tensor> input_tensor(std::move(m_predictor->GetInput(index)));
        input_tensor->Resize(this->m_input_shapes[index]);
    }

	std::cout<<"E"<<std::endl;
    return true;
}

void* ModelRun::getInputPtr(std::string input_name){
    std::unique_ptr<paddle::lite_api::Tensor> tensor = m_predictor->GetInputByName(input_name);
    if(!tensor.get()){
        EAGLEEYE_LOGE("input %s tensor is null", input_name.c_str());
        return NULL;
    }

    return tensor->mutable_data<float>();
}

void ModelRun::setWritablePath(std::string writable_path){
    ModelEngine::setWritablePath(writable_path);

	this->m_nb_path = this->m_model_name;
	if(!endswith(this->m_model_name, ".nb")){
		this->m_nb_path = this->m_nb_path+".nb";
	}
	
	if(endswith(writable_path, "/")){
		this->m_nb_path = writable_path +this->m_nb_path;
	}
	else{
		this->m_nb_path = writable_path + std::string("/")+this->m_nb_path;
	}

	EAGLEEYE_LOGD("paddle model path %s", this->m_nb_path.c_str());
}
}
#endif