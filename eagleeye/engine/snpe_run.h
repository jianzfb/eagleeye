#ifndef _SNPE_MODEL_RUN_H_
#define _SNPE_MODEL_RUN_H_
#ifdef EAGLEEYE_SNPE_SUPPORT
#include <string>
#include <map>
#include <vector>
#include "eagleeye/engine/model_engine.h"
#include "SNPE/SNPE.hpp"
#include "SNPE/SNPEBuilder.hpp"
#include "SNPE/SNPEFactory.hpp"
#include "DlSystem/DlEnums.hpp"
#include "DlSystem/DlVersion.hpp"
#include "DlSystem/IUserBufferFactory.hpp"
#include "DlSystem/IUserBuffer.hpp"
#include "DlSystem/UserBufferMap.hpp"
#include "DlSystem/TensorShape.hpp"
#include "DlSystem/ITensorFactory.hpp"
#include "DlContainer/IDlContainer.hpp"
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include<memory> 

namespace eagleeye{
class ModelRun: public ModelEngine{
public:
	/**
	 * 
	 */
	ModelRun(std::string model_name, 
			 std::string device,
			 std::vector<std::string> input_names=std::vector<std::string>(),
		     std::vector<std::vector<int64_t>> input_shapes=std::vector<std::vector<int64_t>>(),
			 std::vector<std::string> output_names=std::vector<std::string>(),
		     std::vector<std::vector<int64_t>> output_shapes=std::vector<std::vector<int64_t>>(),
		     int omp_num_threads=-1, 
		     int cpu_affinity_policy=-1, 
		     std::string writable_path="/data/local/tmp/");
	/**
	 * 
	 */
	virtual ~ModelRun();

	/**
	 * [run run neural network model]
	 * @param  inputs  {map} <name, data> pair
	 * @param  outputs {map} <name, data> pair
	 * @return         {boolean} true or false
	 */
	virtual bool run(std::map<std::string, unsigned char*> inputs, 
					 std::map<std::string, unsigned char*>& outputs);

	/**
	 * [initialize initialize running environment]
	 * @return {boolean} true or false
	 */
	virtual bool initialize();

	/**
	 * @brief Set the Writable Path object
	 * 
	 * @param writable_path 
	 */
	virtual void setWritablePath(std::string writable_path);

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
	void _run(std::map<std::string, unsigned char*> inputs, 
			  std::map<std::string, unsigned char*>& outputs);

	std::unique_ptr<zdl::DlContainer::IDlContainer> m_container;
	std::unique_ptr<zdl::SNPE::SNPE> m_snpe;

	zdl::DlSystem::TensorMap m_output_tensormap;
	zdl::DlSystem::TensorMap m_input_tensormap;
	std::vector<std::unique_ptr<zdl::DlSystem::ITensor>> m_input_tensors;
	std::string m_dlc_path;

	std::vector<std::shared_ptr<float>> m_temp_ptrs;
};	
}
#endif
#endif