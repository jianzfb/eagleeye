namespace eagleeye{
template<typename Enabled>
ModelRun<PaddleRun, Enabled>::ModelRun(std::string model_name, 
			   std::string device,
			   std::vector<std::string> input_names,
			   std::vector<std::vector<int64_t>> input_shapes,
			   std::vector<std::string> output_names,
			   std::vector<std::vector<int64_t>> output_shapes,
			   int num_threads, 
			   RunPower model_power,
			   std::string writable_path)
	:ModelEngine(model_name,
				 device,
				 input_names,
				 input_shapes,
				 output_names,
				 output_shapes,
				 num_threads,
				 model_power,
				 writable_path){    
	// paddle model file path
	this->m_nb_name = model_name+".nb";
	this->m_is_init = false;
}    

template<typename Enabled>
ModelRun<PaddleRun, Enabled>::~ModelRun(){
	// do nothing
}

template<typename Enabled>
bool ModelRun<PaddleRun, Enabled>::run(std::map<std::string, unsigned char*> inputs, 
				   std::map<std::string, unsigned char*>& outputs){	
    // ignore outside inputs
    if(!this->m_predictor.get()){
        EAGLEEYE_LOGE("Paddle predictor null.");
        return false;
    }

	// set input
    for(int index=0; index < this->m_input_names.size(); ++index){
        // 输入节点名字
        std::string node_name = this->m_input_names[index];
		if(inputs.find(node_name) == inputs.end()){
			continue;
		}

        // 输入节点形状
        std::vector<int64_t> node_shape = this->m_input_shapes[index];

        // 输入节点数据
        unsigned char* node_data = inputs[node_name];

        // 预处理参数 (仅支持3通道图像数据)
		if(node_shape[1] == 3 && (this->m_input_convert_params.find(node_name) != this->m_input_convert_params.end())){
			ConvertParam convert_param = this->m_input_convert_params[node_name];
			float* inner_ptr = (float*)(this->getInputPtr(node_name));
			if(convert_param.reverse_channel){
				this->bgrToRgbTensorCHW(node_data, 
							inner_ptr, 
							node_shape[3], 
							node_shape[2], 
							convert_param.bias.data(), 
							convert_param.scale.data());
			}
			else{
				this->bgrToTensorCHW(node_data, 
							inner_ptr, 
							node_shape[3], 
							node_shape[2], 
							convert_param.bias.data(), 
							convert_param.scale.data());
			}
		}
    }

    // run model
    this->m_predictor->Run();

    // get output from net
	if(outputs.size() > 0){
		std::map<std::string, unsigned char*>::iterator iter, iend(outputs.end());
		for(iter = outputs.begin(); iter != iend; ++iter){
			std::unique_ptr<const paddle::lite_api::Tensor> s = 
				this->m_predictor->GetOutput(this->m_output_map_index[iter->first]);
			iter->second = (unsigned char*)(s->mutable_data<float>());
		}
	}
    return true;
}

template<typename Enabled>
bool ModelRun<PaddleRun, Enabled>::initialize(){
    if(this->m_is_init){
        EAGLEEYE_LOGD("PADDLELITE Has been finished model initialize.");
        return true;
    }
    this->m_is_init = true;

    std::string model_folder = this->getModelFolder();
    if(model_folder == ""){
        EAGLEEYE_LOGD("PADDLELITE Dont set model folder.");
        return false;
    }

    // 1. Set MobileConfig
    paddle::lite_api::MobileConfig config;
	std::string nb_path;
    if(endswith(model_folder, "/")){
        nb_path = model_folder + this->m_nb_name;
    }
    else{
        nb_path = model_folder + "/" + this->m_nb_name;
    }
    EAGLEEYE_LOGD("PADDLELITE Set paddle model file %s", nb_path.c_str());
    config.set_model_from_file(nb_path);

	if(this->m_device == "GPU"){
#if defined(__ANDROID__) || defined(ANDROID)		
		bool is_opencl_backend_valid = paddle::lite_api::IsOpenCLBackendValid();		
		if (!is_opencl_backend_valid) {
			EAGLEEYE_LOGE("Unsupport opencl nb model.");
			return false;
		}

		std::string kernel_bin_path = "";
		if(endswith(this->m_writable_path, "/")){
			kernel_bin_path = this->m_writable_path + m_model_name + "_kernel.bin";
		}
		else{
			kernel_bin_path = this->m_writable_path + "/" + m_model_name + "_kernel.bin";
		}
		if(isfileexist(kernel_bin_path.c_str())){
			config.set_opencl_binary_path_name(this->m_writable_path, m_model_name+"_kernel.bin");
		}
		else{
			static bool run_once = false;
			if(!run_once){
				config.set_opencl_binary_path_name(this->m_writable_path, m_model_name+"_kernel.bin");
				run_once = true;
			}
		}
		// config.set_opencl_tune(true);
#endif
	}

    // NOTE: To load model transformed by model_optimize_tool before
    // release/v2.3.0, plese use `set_model_dir` API as listed below.
    // config.set_model_dir(model_dir);
	paddle::lite_api::PowerMode power_mode = paddle::lite_api::LITE_POWER_HIGH;
	if(this->m_model_power == HIGH_POWER){
		power_mode = paddle::lite_api::LITE_POWER_HIGH;
	}
	else if(this->m_model_power == LOW_POWER){	
		power_mode = paddle::lite_api::LITE_POWER_LOW;
	}
	else{
		power_mode = paddle::lite_api::LITE_POWER_NO_BIND;
	}
    config.set_power_mode(power_mode);
    config.set_threads(this->m_omp_num_threads);

    // 2. Create PaddlePredictor by MobileConfig
    EAGLEEYE_LOGD("PADDLELITE Create paddle predictor.");
    m_predictor = paddle::lite_api::CreatePaddlePredictor<paddle::lite_api::MobileConfig>(config);
    if(!m_predictor.get()){
		EAGLEEYE_LOGD("PADDLELITE Fail get paddle predictor.");
        return false;
    }
	std::string version = m_predictor->GetVersion();
	EAGLEEYE_LOGD("PADDLELITE PaddleLite version %s.", version.c_str());

    // 3.step resize tensor
    EAGLEEYE_LOGD("PADDLELITE Initialize Input tensor.");
    for(int index = 0; index < this->m_input_names.size(); ++index){
        std::unique_ptr<paddle::lite_api::Tensor> input_tensor(std::move(m_predictor->GetInput(index)));
		if(this->m_input_shapes[index][0] != -1){
			input_tensor->Resize(this->m_input_shapes[index]);
		}
	}
    return true;
}

template<typename Enabled>
void* ModelRun<PaddleRun, Enabled>::getInputPtr(std::string input_name){
    std::unique_ptr<paddle::lite_api::Tensor> tensor =
			 m_predictor->GetInputByName(input_name);
    if(!tensor.get()){
        EAGLEEYE_LOGE("PADDLELITE Input %s tensor is null.", input_name.c_str());
        return NULL;
    }

    return tensor->mutable_data<float>();
}

template<typename Enabled>
const void* ModelRun<PaddleRun, Enabled>::getOutputPtr(std::string output_name){
    std::unique_ptr<const paddle::lite_api::Tensor> tensor = 
			m_predictor->GetOutput(this->m_output_map_index[output_name]);
    return tensor->mutable_data<float>();
}

// template<typename Enabled>
// std::shared_ptr<const paddle::lite_api::Tensor> ModelRun<PaddleRun, Enabled>::getOutput(std::string output_name){
// 	return std::move(this->m_predictor->GetOutput(this->m_output_map_index[output_name]));
// }

// template<typename Enabled>
// std::shared_ptr<paddle::lite_api::Tensor> ModelRun<PaddleRun, Enabled>::getInput(std::string input_name){
// 	return std::move(this->m_predictor->GetInputByName(input_name));
// }

template<typename Enabled>
void ModelRun<PaddleRun, Enabled>::resize(std::string input_name, std::vector<int64_t> shape){
    std::unique_ptr<paddle::lite_api::Tensor> tensor =
			 m_predictor->GetInputByName(input_name);
	tensor->Resize(shape);
}

template<typename Enabled>
void ModelRun<PaddleRun, Enabled>::getInput(std::string input_name, void*& input_ptr, std::vector<int64_t>& shape){
	std::unique_ptr<const paddle::lite_api::Tensor> tensor = 
				m_predictor->GetInputByName(input_name);
	input_ptr = (void*)(tensor->data<float>());
    shape = tensor->shape();
}

template<typename Enabled>
void ModelRun<PaddleRun, Enabled>::getOutput(std::string output_name, void*& output_ptr, std::vector<int64_t>& shape){
	std::unique_ptr<const paddle::lite_api::Tensor> tensor = 
				this->m_predictor->GetOutput(this->m_output_map_index[output_name]);

	output_ptr = (void*)(tensor->data<float>());
    shape = tensor->shape();
}

}