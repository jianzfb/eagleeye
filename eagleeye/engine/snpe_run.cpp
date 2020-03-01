#ifdef EAGLEEYE_SNPE_SUPPORT
#include "eagleeye/engine/snpe_run.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeLog.h"
#include <sys/time.h>
#include <algorithm>
#include <getopt.h>
#include <cstdlib>
#include <unordered_map>
#include <stdio.h>
#include <assert.h>
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
	// dlc file path
	this->m_dlc_path = model_name;
	if(!endswith(model_name, ".dlc")){
		this->m_dlc_path = this->m_dlc_path+".dlc";
	}
	if(writable_path.length() == 0){
		if(!ispath(this->m_dlc_path)){
			this->m_dlc_path = this->getModelRoot() + std::string("/")+this->m_dlc_path;
		}
	}
	else{
		if(endswith(writable_path, "/")){
			this->m_dlc_path = writable_path +this->m_dlc_path;
		}
		else{
			this->m_dlc_path = writable_path + std::string("/")+this->m_dlc_path;
		}
	}
	EAGLEEYE_LOGD("snpe dlc %s", this->m_dlc_path.c_str());
}

ModelRun::~ModelRun(){
}

bool ModelRun::run(std::map<std::string, unsigned char*> inputs, 
				   std::map<std::string, unsigned char*>& outputs){
	
	if(this->isDynamicInputShape()){
		// for dynamic input, only support slice_num is not fixed
		int slice_num = this->m_input_shapes[0][0];

		// slice inputs
		std::vector<std::map<std::string, unsigned char*>> slice_inputs;
		slice_inputs.resize(slice_num);

		for(int index=0; index<this->m_input_names.size(); ++index){
			std::string input_name = this->m_input_names[index];
			// from model
			std::string tensor_name = input_name+":0";
			auto tensor_ptr = this->m_input_tensormap.getTensor(tensor_name.c_str());
			zdl::DlSystem::TensorShape shape = tensor_ptr->getShape();
			int model_shape_size = 1;
			for(int i=0; i<shape.rank(); ++i){
				model_shape_size *= shape[i];
			}

			// from input
			int slice_input_size = 1;
			for(int i=1; i<this->m_input_shapes[index].size(); ++i){
				slice_input_size *= this->m_input_shapes[index][i];
			}
			// assert(shape_size == slice_input_size);
			// EAGLEEYE_LOGD("model size %s %s", eagleeye::tos(model_shape_size).c_str(),eagleeye::tos(slice_input_size).c_str());

			// set slice_inputs
			for(int s_i=0; s_i<slice_num; ++s_i){
				slice_inputs[s_i][input_name] = (unsigned char*)(((float*)inputs[input_name])+s_i*slice_input_size);
			}
		}

		this->m_temp_ptrs.clear();
		std::vector<int> slice_output_size;
		for(int index=0; index<this->m_output_names.size(); ++index){
			int output_shape_size = 1;
			for(int i=0; i<this->m_output_shapes[index].size(); ++i){
				output_shape_size *= this->m_output_shapes[index][i];
			}

			this->m_temp_ptrs.push_back(std::shared_ptr<float>(new float[output_shape_size], std::default_delete<float[]>()));
			slice_output_size.push_back(output_shape_size / slice_num);
		}

		for(int s_i=0; s_i<slice_num; ++s_i){
			std::map<std::string, unsigned char*> single_slice_outputs;
			this->_run(slice_inputs[s_i], single_slice_outputs);

			for(int index=0; index<this->m_output_names.size(); ++index){
				std::string output_name = this->m_output_names[index];
				memcpy(this->m_temp_ptrs[index].get()+s_i*slice_output_size[index], 
					   single_slice_outputs[output_name], 
					   sizeof(float)*slice_output_size[index]);
			}
		}

		for(int index=0; index<this->m_output_shapes.size(); ++index){
			outputs[this->m_output_names[index]] = 
						(unsigned char*)this->m_temp_ptrs[index].get();
		}		
	}
	else{
		this->_run(inputs, outputs);
	}
	return true;
}

bool ModelRun::initialize(){
	// step 1. set runtime
	zdl::DlSystem::Runtime_t runtime = this->checkRuntime();
	// step 2. load dlc
	EAGLEEYE_LOGD("start load dlc %s", this->m_dlc_path.c_str());
	this->m_container = loadContainerFromFile(this->m_dlc_path);
	if(this->m_container.get() == NULL){
		EAGLEEYE_LOGE("fail to load dlc %s", this->m_dlc_path.c_str());
		return false;
	}
	EAGLEEYE_LOGD("success to load dlc %s", this->m_dlc_path.c_str());

	// step 3. prepare SNPE
	EAGLEEYE_LOGD("build snpe engine");
	zdl::DlSystem::PlatformConfig platformConfig;
	bool result = setBuilderOptions(this->m_container, runtime, false, platformConfig);
	if(!result){
		return false;
	}

	// step 4. prepare tensors
	EAGLEEYE_LOGD("create dlc input tensor");
	this->createInputTensors();
	EAGLEEYE_LOGD("success to model initialize");
	return true;
}

zdl::DlSystem::Runtime_t ModelRun::checkRuntime(){
    static zdl::DlSystem::Runtime_t runtime;
    std::stringstream path;
	if(this->m_writable_path.length() == 0){
	    path << "/data/local/tmp/dsp/lib;/vendor/lib64/;/system/lib/rfsa/adsp;/system/vendor/lib/rfsa/adsp;/dsp";
	}
	else{
		path << this->m_writable_path + ";/data/local/tmp/dsp/lib;/vendor/lib64/;/system/lib/rfsa/adsp;/system/vendor/lib/rfsa/adsp;/dsp";
	}
    setenv("ADSP_LIBRARY_PATH", path.str().c_str(), 1 /*override*/);

    if(this->m_device == "CPU"){
    	runtime = zdl::DlSystem::Runtime_t::CPU;
    }
    else if(this->m_device == "GPU"){
    	runtime = zdl::DlSystem::Runtime_t::GPU;
    }
    else if(this->m_device == "DSP"){
    	runtime = zdl::DlSystem::Runtime_t::DSP;
    }
    else if(this->m_device == "NPU"){
    	runtime = zdl::DlSystem::Runtime_t::AIP_FIXED8_TF;
    }
    else{
    	EAGLEEYE_LOGE("dont support %s runtime", this->m_device.c_str());
 		runtime = zdl::DlSystem::Runtime_t::UNSET;
    }

    return runtime;
}

std::unique_ptr<zdl::DlContainer::IDlContainer> ModelRun::loadContainerFromFile(std::string containerPath){
	std::unique_ptr<zdl::DlContainer::IDlContainer> container;
    container = zdl::DlContainer::IDlContainer::open(zdl::DlSystem::String(containerPath.c_str()));
    return std::move(container);
}

bool ModelRun::setBuilderOptions(std::unique_ptr<zdl::DlContainer::IDlContainer>& container,
                                                   zdl::DlSystem::Runtime_t runtime,
                                                   bool useUserSuppliedBuffers,
						   						   zdl::DlSystem::PlatformConfig platformConfig){
	std::vector<std::string> output_names = this->m_output_names;
	if(this->m_output_name_map.size() > 0){
		for(int i=0; i<output_names.size(); ++i){
			output_names[i] = this->m_output_name_map[output_names[i]];
		}
	}
	zdl::DlSystem::StringList snpe_output_names;
	for(int i=0; i<output_names.size(); ++i){
		snpe_output_names.append(output_names[i].c_str());
		EAGLEEYE_LOGD("output node %s", output_names[i].c_str());
	}

    zdl::DlSystem::UDLBundle udlBundle;
    zdl::SNPE::SNPEBuilder snpeBuilder(container.get());
    this->m_snpe = snpeBuilder.setOutputLayers(snpe_output_names)
								.setRuntimeProcessor(runtime)
								.setUdlBundle(udlBundle)
								.setUseUserSuppliedBuffers(useUserSuppliedBuffers)
								.setPlatformConfig(platformConfig)
								.build();
	if(this->m_snpe.get() == NULL){
		EAGLEEYE_LOGE("fail to build snpe");
		return false;
	}
	return true;
}

void ModelRun::createInputTensors(){
    const auto &strlist_name = this->m_snpe->getInputTensorNames();
    if (!strlist_name) throw std::runtime_error("Error obtaining Input tensor names");

	EAGLEEYE_LOGD("create input tensors number %d",(*strlist_name).size());

    for(int index=0; index<(*strlist_name).size(); ++index){
    	std::string name = (*strlist_name).at(index);
		EAGLEEYE_LOGD("create input tensor %s",name.c_str());
    	const auto &inputdims_opt = this->m_snpe->getInputDimensions((*strlist_name).at(index));
	    const auto &inputshape = *inputdims_opt;
		std::unique_ptr<zdl::DlSystem::ITensor> tensor = zdl::SNPE::SNPEFactory::getTensorFactory().createTensor(inputshape);
    	
    	this->m_input_tensormap.add(name.c_str(), tensor.get());
    	this->m_input_tensors.push_back(std::move(tensor));
    }
}

void ModelRun::_run(std::map<std::string, unsigned char*> inputs, 
			  		std::map<std::string, unsigned char*>& outputs){
	// step 1. load input tensor
	zdl::DlSystem::StringList input_tensor_names = this->m_input_tensormap.getTensorNames();
	std::for_each(input_tensor_names.begin(), input_tensor_names.end(), [&](const char* tensor_name){
		EAGLEEYE_LOGD("snpe model input tensor name %s", tensor_name);
		auto tensor_ptr = this->m_input_tensormap.getTensor(tensor_name);
		zdl::DlSystem::TensorShape shape = tensor_ptr->getShape();
		int shape_size = 1;
		for(int i=0; i<shape.rank(); ++i){
			shape_size *= shape[i];
		}

		std::string name = tensor_name;
		name = name.substr(0, name.length()-2);
		memcpy(tensor_ptr->begin().dataPointer(), inputs[name], sizeof(float)* shape_size);
	});

	// step 2. execute model
	//Execute the network and store the outputs that were specified when creating the network in a TensorMap
	EAGLEEYE_LOGD("execute snpe model");
    this->m_snpe->execute(this->m_input_tensormap, this->m_output_tensormap);

    // step 3. fill output
    zdl::DlSystem::StringList output_tensor_names = this->m_output_tensormap.getTensorNames();
    std::for_each( output_tensor_names.begin(), output_tensor_names.end(), [&](const char* tensor_name){
		// EAGLEEYE_LOGD("snpe model output tensor name %s", tensor_name);
        auto tensor_ptr = this->m_output_tensormap.getTensor(tensor_name);
        zdl::DlSystem::TensorShape shape = tensor_ptr->getShape();

       	std::string name = tensor_name;
		name = name.substr(0, name.length()-2);

		std::string transformed_name = name;
		if(this->m_inv_output_name_map2.size() > 0){
			assert(this->m_inv_output_name_map2.find(name) != this->m_inv_output_name_map2.end());
			transformed_name = this->m_inv_output_name_map2[name];
		}
		// EAGLEEYE_LOGD("pair %s and %s",tensor_name, transformed_name.c_str());
        outputs[transformed_name] = (unsigned char*)tensor_ptr->begin().dataPointer();
	});
}

void ModelRun::setWritablePath(std::string writable_path){
	ModelEngine::setWritablePath(writable_path);

	this->m_dlc_path = this->m_model_name;
	if(!endswith(this->m_model_name, ".dlc")){
		this->m_dlc_path = this->m_dlc_path+".dlc";
	}
	
	if(endswith(writable_path, "/")){
		this->m_dlc_path = writable_path +this->m_dlc_path;
	}
	else{
		this->m_dlc_path = writable_path + std::string("/")+this->m_dlc_path;
	}

	EAGLEEYE_LOGD("dlc path %s", this->m_dlc_path.c_str());
}
}

#endif