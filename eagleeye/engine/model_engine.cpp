#include "eagleeye/engine/model_engine.h"
#if defined(__ANDROID__) || defined(ANDROID)
#pragma message( "IN ANDROID arm" )
#include "eagleeye/engine/math/arm/preprocess.h"
#else
#pragma message( "IN X86")
#include "eagleeye/engine/math/x86/preprocess.h"
#endif
#include <iostream>
#include <dlfcn.h>
#include <stdint.h>


namespace eagleeye{
static void where_am_i() {}		
ModelEngine::ModelEngine(std::string model_name, 
				std::string device, 
				std::vector<std::string> input_names,
		     	std::vector<std::vector<int64_t>> input_shapes,
		     	std::vector<std::string> output_names,
		     	std::vector<std::vector<int64_t>> output_shapes,
		     	int omp_num_threads,
		     	RunPower model_power,
		     	std::string writable_path){
	this->m_omp_num_threads = omp_num_threads;
  this->m_model_power = model_power;
	this->m_writable_path = writable_path;
	this->m_device = device;
	this->m_model_name = model_name;

	this->m_input_names = input_names;
	this->m_output_names = output_names;

	for(int index=0; index < input_names.size(); ++index){
		m_input_map_index[input_names[index]] = index;
	}
	for(int index=0; index < output_names.size(); ++index){
		m_output_map_index[output_names[index]] = index;
	}

	this->m_is_dynamic_input_shape = false;
	this->m_is_dynamic_output_shape = false;
	for(int i=0; i<input_shapes.size(); ++i){
		for(int j=0; j<input_shapes[i].size(); ++j){
			if(input_shapes[i][j] < 0){
				this->m_is_dynamic_input_shape = true;
			}
		}
	}
	if(!this->m_is_dynamic_input_shape){
		this->m_input_shapes = input_shapes;
	}
	// if(input_shapes.size() == 0){
	// 	this->m_is_dynamic_input_shape = true;
	// }

	for(int i=0; i<output_shapes.size(); ++i){
		for(int j=0; j<output_shapes[i].size(); ++j){
			if(output_shapes[i][j] < 0){
				this->m_is_dynamic_output_shape = true;
			}
		}
	}
	if(!this->m_is_dynamic_output_shape){
		this->m_output_shapes = output_shapes;
	}
	// if(output_shapes.size() == 0){
	// 	this->m_is_dynamic_output_shape = true;
	// }

	Dl_info info;
	dladdr((void*)&where_am_i, &info);
	this->m_root = info.dli_fname;
	this->m_root = this->m_root.substr(0, this->m_root.rfind('/'));
}

ModelEngine::~ModelEngine(){
}

void ModelEngine::setModelFolder(std::string model_folder){
  this->m_model_folder = model_folder;
}

std::string ModelEngine::getModelFolder(){
  return this->m_model_folder;
}

bool ModelEngine::isDynamicInputShape(){
	return this->m_is_dynamic_input_shape;
}

bool ModelEngine::isDynamicOutputShape(){
	return this->m_is_dynamic_output_shape;
}

void ModelEngine::setInputShapes(std::vector<std::vector<int64_t>> input_shapes){
	this->m_input_shapes = input_shapes;
}

void ModelEngine::setInputNames(std::vector<std::string> input_names){
	this->m_input_names = input_names;
}

std::vector<std::string> ModelEngine::getInputNames(){
	return this->m_input_names;
}

void ModelEngine::setOutputShapes(std::vector<std::vector<int64_t>> output_shapes){
	this->m_output_shapes = output_shapes;
}

void ModelEngine::setOutputNames(std::vector<std::string> output_names){
	this->m_output_names = output_names;
}

std::vector<std::string> ModelEngine::getOutputNames(){
	return this->m_output_names;
}

void ModelEngine::setOutputNameMap(std::map<std::string, std::string> name_map){
	this->m_output_name_map = name_map; 
	this->m_output_name_map2 = name_map; 

	std::map<std::string,std::string>::iterator iter;
	for(iter=name_map.begin(); iter != name_map.end(); ++iter){
		this->m_inv_output_name_map[iter->second] = iter->first;
		this->m_inv_output_name_map2[iter->second] = iter->first;
	}
}

void ModelEngine::setOutputNameMap2(std::map<std::string, std::string> name_map){
	this->m_output_name_map2 = name_map; 

	std::map<std::string,std::string>::iterator iter;
	for(iter=name_map.begin(); iter != name_map.end(); ++iter){
		this->m_inv_output_name_map2[iter->second] = iter->first;
	}
}

void ModelEngine::setInputNameMap(std::map<std::string, std::string> name_map){
	this->m_input_name_map = name_map;
		std::map<std::string,std::string>::iterator iter;
	for(iter=name_map.begin(); iter != name_map.end(); ++iter){
		this->m_inv_input_name_map[iter->second] = iter->first;
	}
}

std::string ModelEngine::getModelRoot(){
	return this->m_root;
}

void ModelEngine::setWritablePath(std::string writable_path){
	this->m_writable_path = writable_path;
}

std::string ModelEngine::getWritablePath(){
	return this->m_writable_path;
}


void ModelEngine::bgrToTensorCHW(const uint8_t* src,
                       float* output,
                       int width,
                       int height,
                       float* means,
                       float* scales) {
#if defined(__ANDROID__) || defined(ANDROID)                         
  math::arm::bgrToTensorCHW(src, output, width, height, means, scales);
#else
  math::x86::bgrToTensorCHW(src, output, width, height, means, scales);
#endif
}

void ModelEngine::bgrToRgbTensorCHW(const uint8_t* src,
                       float* output,
                       int width,
                       int height,
                       float* means,
                       float* scales) {
#if defined(__ANDROID__) || defined(ANDROID)                            
  math::arm::bgrToRgbTensorCHW(src, output, width, height, means, scales);
#else
  math::x86::bgrToRgbTensorCHW(src, output, width, height, means, scales);
#endif
}
}