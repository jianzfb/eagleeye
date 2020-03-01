#include "eagleeye/engine/mace_run.h"
#ifdef EAGLEEYE_MACE_SUPPORT
#include <dirent.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <numeric>
#include "mace/public/mace.h"
#include "mace/public/mace_engine_factory.h"
#include "eagleeye/common/EagleeyeFile.h"
#include "eagleeye/common/EagleeyeTime.h"
#ifdef MACE_ENABLE_OPENCL
namespace mace {
const unsigned char *LoadOpenCLBinary();
size_t OpenCLBinarySize();
const unsigned char *LoadOpenCLParameter();
size_t OpenCLParameterSize();
}  // namespace mace
#endif

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
	this->m_omp_num_threads = omp_num_threads;
	this->m_cpu_affinity_policy = cpu_affinity_policy;

	this->m_model_name = model_name;
	this->m_device = device;
	this->m_opencl_storage_path = std::string("");
	this->m_input_names = input_names;
	this->m_input_shapes = input_shapes;
	this->m_output_names = output_names;
	this->m_output_shapes = output_shapes;

	this->m_is_ready = false;
  this->m_writable_path = writable_path;

	if(this->m_writable_path[this->m_writable_path.length() - 1] == '\\' || 
	   this->m_writable_path[this->m_writable_path.length() - 1] == '/'){
	   this->m_writable_path = this->m_writable_path.substr(0,this->m_writable_path.length() -1);
  }
}
ModelRun::~ModelRun(){
}

mace::DeviceType ModelRun::parseDeviceType(const std::string device_str){
	if (device_str.compare("CPU") == 0) {
	    return mace::DeviceType::CPU;
	} else if (device_str.compare("GPU") == 0) {
	    return mace::DeviceType::GPU;
	} else if (device_str.compare("HEXAGON") == 0) {
	    return mace::DeviceType::HEXAGON;
	} else if(device_str.compare("DSP") == 0){
      return mace::DeviceType::HEXAGON;
  }
  else {
	    return mace::DeviceType::CPU;
	}
}

bool ModelRun::initialize(){
  // load model
  mace::DeviceType device_type = this->parseDeviceType(this->m_device);
  // configuration
  // Detailed information please see mace.h
  mace::MaceStatus status;
  mace::MaceEngineConfig config(device_type);
  status = config.SetCPUThreadPolicy(
      this->m_omp_num_threads,
      static_cast<mace::CPUAffinityPolicy>(mace::AFFINITY_BIG_ONLY));
  if (status != mace::MaceStatus::MACE_SUCCESS) {
    std::cerr << "Set openmp or cpu affinity failed." << std::endl;
    this->m_is_ready = false;
    return this->m_is_ready;
  }

#ifdef MACE_ENABLE_OPENCL
  std::shared_ptr<mace::GPUContext> gpu_context;
  if (device_type == mace::DeviceType::GPU) {
    std::string storage_path = this->m_writable_path;
    if(isdirexist(storage_path.c_str()) == false){
      // dont exist, and mkdir
      createdirectory(storage_path.c_str());
    }

    std::string opencl_binary_file = storage_path;    
    std::vector<std::string> opencl_binary_paths = {opencl_binary_file};

    gpu_context = mace::GPUContextBuilder()
        .SetStoragePath(storage_path)
    	.SetOpenCLBinaryPaths(opencl_binary_paths)
    	.SetOpenCLParameterPath(opencl_binary_file)
        .Finalize();

    config.SetGPUContext(gpu_context);
    config.SetGPUHints(
        static_cast<mace::GPUPerfHint>(mace::GPUPerfHint::PERF_HIGH),
        static_cast<mace::GPUPriorityHint>(mace::GPUPriorityHint::PRIORITY_HIGH));
  }
#endif  // MACE_ENABLE_OPENCL

  // Create Engine
  mace::MaceStatus create_engine_status;
  std::string model_data_file="";
  create_engine_status =
      CreateMaceEngineFromCode(this->m_model_name,
                               nullptr,
                               0,
                               this->m_input_names,
                               this->m_output_names,
                               config,
                               &this->m_engine);

  if (create_engine_status != mace::MaceStatus::MACE_SUCCESS) {
    std::cerr << "Create engine error, please check the arguments first, "
              << "if correct, the device may not run the model, "
              << "please fall back to other strategy."
              << std::endl;

    this->m_is_ready = false;
    return this->m_is_ready;
  }

  if(!this->isDynamicInputShape()){
    const size_t input_count = this->m_input_names.size();
    for (size_t i = 0; i < input_count; ++i) {
      int64_t input_size =
          std::accumulate(this->m_input_shapes[i].begin(), this->m_input_shapes[i].end(), 1,
                          std::multiplies<int64_t>());
      this->m_inputs_size[this->m_input_names[i]] = input_size;

      auto buffer_in = std::shared_ptr<float>(new float[input_size],
                                              std::default_delete<float[]>());
      this->m_inputs[this->m_input_names[i]] = mace::MaceTensor(this->m_input_shapes[i], buffer_in);
    }
  }

  if(!this->isDynamicOutputShape()){
    const size_t output_count = this->m_output_names.size();
    for (size_t i = 0; i < output_count; ++i) {
      int64_t output_size =
          std::accumulate(this->m_output_shapes[i].begin(), this->m_output_shapes[i].end(), 1,
                          std::multiplies<int64_t>());
      auto buffer_out = std::shared_ptr<float>(new float[output_size],
                                               std::default_delete<float[]>());
      this->m_outputs[this->m_output_names[i]] = mace::MaceTensor(this->m_output_shapes[i], buffer_out);
    }
  }

  this->m_is_ready = true;
  return this->m_is_ready;
}

bool ModelRun::run(std::map<std::string, unsigned char*> inputs, 
				           std::map<std::string, unsigned char*>& outputs){
	if(this->m_is_ready == false){
		return false;
	}
  // dynamic set input shape
  if(this->isDynamicInputShape()){
    const size_t input_count = this->m_input_names.size();
    for (size_t i = 0; i < input_count; ++i) {
      int64_t input_size =
          std::accumulate(this->m_input_shapes[i].begin(), this->m_input_shapes[i].end(), 1,
                          std::multiplies<int64_t>());
      this->m_inputs_size[this->m_input_names[i]] = input_size;

      auto buffer_in = std::shared_ptr<float>(new float[input_size],
                                              std::default_delete<float[]>());
      this->m_inputs[this->m_input_names[i]] = mace::MaceTensor(this->m_input_shapes[i], buffer_in);
    }
  }

  // dynamic set output shape
  if(this->isDynamicOutputShape()){
    const size_t output_count = this->m_output_names.size();
    for (size_t i = 0; i < output_count; ++i) {
      int64_t output_size =
          std::accumulate(this->m_output_shapes[i].begin(), this->m_output_shapes[i].end(), 1,
                          std::multiplies<int64_t>());
      auto buffer_out = std::shared_ptr<float>(new float[output_size],
                                               std::default_delete<float[]>());
      this->m_outputs[this->m_output_names[i]] = mace::MaceTensor(this->m_output_shapes[i], buffer_out);
    }
  }

  // check data
	if(inputs.size() != this->m_input_shapes.size()){
		std::cerr <<"data number is not consistent with model input"<< std::endl;
		return false;
	}
 	
 	/**
 	 * copy data to mace tensor
 	 */
 	std::map<std::string, unsigned char*>::iterator iter;
	for(iter = inputs.begin(); iter != inputs.end(); iter++){
		memcpy(this->m_inputs[iter->first].data().get(), iter->second, sizeof(float)* this->m_inputs_size[iter->first]);
  }

	/**
	 * run mace model
	 */
	this->m_engine->Run(this->m_inputs, &this->m_outputs);

	/**
	 * extract mace output tensor data pointer
	 */
	std::map<std::string, mace::MaceTensor>::iterator o_iter;
	for(o_iter=this->m_outputs.begin(); o_iter != this->m_outputs.end(); o_iter++){
		outputs[o_iter->first] = (unsigned char*)(this->m_outputs[o_iter->first].data().get());
  }

	return true;
}

void ModelRun::setWritablePath(std::string writable_path){
	this->m_writable_path = writable_path;
	if(this->m_writable_path[this->m_writable_path.length() - 1] == '\\' || 
	   this->m_writable_path[this->m_writable_path.length() - 1] == '/'){
	   this->m_writable_path = this->m_writable_path.substr(0,this->m_writable_path.length() -1);
  }
}
}
#endif