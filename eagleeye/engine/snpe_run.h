#ifndef _SNPE_MODEL_RUN_H_
#define _SNPE_MODEL_RUN_H_
#include "eagleeye/engine/model_engine.h"
#include "eagleeye/engine/model_run.h"
#include "SNPE/SNPE.hpp"
#include "SNPE/SNPEBuilder.hpp"
#include "SNPE/SNPEFactory.hpp"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeTime.h"
#include "DlSystem/DlEnums.hpp"
#include "DlSystem/DlVersion.hpp"
#include "DlSystem/IUserBufferFactory.hpp"
#include "DlSystem/IUserBuffer.hpp"
#include "DlSystem/UserBufferMap.hpp"
#include "DlSystem/TensorShape.hpp"
#include "DlSystem/ITensorFactory.hpp"
#include "DlContainer/IDlContainer.hpp"
#include "eagleeye/common/EagleeyeFile.h"
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include<memory> 
#include <sys/time.h>
#include <algorithm>
#include <getopt.h>
#include <cstdlib>
#include <unordered_map>
#include <stdio.h>
#include <assert.h>
namespace eagleeye{

template<typename Enabled>
class ModelRun<SnpeRun, Enabled>: public ModelEngine{
public:
	/**
	 * [constructor: build snpe model]
	 */
	ModelRun(std::string model_name, 
			 std::string device,
			 std::vector<std::string> input_names=std::vector<std::string>(),
		     std::vector<std::vector<int64_t>> input_shapes=std::vector<std::vector<int64_t>>(),
			 std::vector<std::string> output_names=std::vector<std::string>(),
		     std::vector<std::vector<int64_t>> output_shapes=std::vector<std::vector<int64_t>>(),
		     int num_threads = -1, 
		     RunPower model_power = HIGH_POWER, 
		     std::string writable_path="/data/local/tmp/",
			 bool inner_preprocess=false);
	/**
	 * [destructor]
	 */
	virtual ~ModelRun();

	/**
	 * [run neural network model]
	 * @param  inputs  {map} <name, data> pair
	 * @param  outputs {map} <name, data> pair
	 * @return         {boolean} true or false
	 */
	virtual bool run(std::map<std::string, const unsigned char*> inputs, 
					 std::map<std::string, unsigned char*>& outputs);

	/**
	 * [initialize initialize running environment]
	 * @return {boolean} true or false
	 */
	virtual bool initialize();

	/**
	 * @brief Get the Input Ptr object
	 * 
	 * @param input_name 
	 * @return void* 
	 */
	virtual void* getInputPtr(std::string input_name);

protected:
	/**
	 * [checkRuntime check snpe runtime]
	 * @return [description]
	 */
	zdl::DlSystem::Runtime_t checkRuntime();

	/**
	 * [loadContainerFromFile load model from dlc]
	 * @return [model container]
	 */
	std::unique_ptr<zdl::DlContainer::IDlContainer> loadContainerFromFile(std::string containerPath);

	/**
	 * [setBuilderOptions build snpe engine]
	 */
	bool setBuilderOptions(std::unique_ptr<zdl::DlContainer::IDlContainer>& container,
	                       zdl::DlSystem::Runtime_t runtime,
	                       bool useUserSuppliedBuffers,
	                       zdl::DlSystem::PlatformConfig platformConfig);

	/**
	 * [loadInputTensor create input tensor controled by SNPE]
	 */
	void createInputTensors();

	/**
	 * [_run description]
	 * @param inputs  [description]
	 * @param outputs [description]
	 */
	bool _run(std::map<std::string, const unsigned char*> inputs, 
			  std::map<std::string, unsigned char*>& outputs);

	std::unique_ptr<zdl::DlContainer::IDlContainer> m_container;
	std::unique_ptr<zdl::SNPE::SNPE> m_snpe;

	zdl::DlSystem::TensorMap m_output_tensormap;
	zdl::DlSystem::TensorMap m_input_tensormap;
	std::vector<std::unique_ptr<zdl::DlSystem::ITensor>> m_input_tensors;

	std::string m_model_name;
	std::vector<std::shared_ptr<float>> m_temp_ptrs;
	bool m_is_init;
	bool m_inner_preprocess;
};	
}
#include "eagleeye/engine/snpe_run.hpp"
#endif