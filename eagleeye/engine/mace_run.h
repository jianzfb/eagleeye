#ifndef _MACE_MODEL_RUN_H_
#define _MACE_MODEL_RUN_H_
#include <string>
#include <map>
#include <vector>
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
#include "eagleeye/common/EagleeyeFile.h"
#include "eagleeye/common/EagleeyeTime.h"
#include "eagleeye/engine/model_engine.h"
#include "mace/public/mace.h"
#include "mace/public/mace_engine_factory.h"
namespace eagleeye{
class MaceRun: public ModelEngine{
public:
	/**
	 * [constructor: build mace model]
	 */
	MaceRun(std::string model_name, 
			 std::string device,
			 std::vector<std::string> input_names=std::vector<std::string>(),
		     std::vector<std::vector<int64_t>> input_shapes=std::vector<std::vector<int64_t>>(),
			 std::vector<std::string> output_names=std::vector<std::string>(),
		     std::vector<std::vector<int64_t>> output_shapes=std::vector<std::vector<int64_t>>(),
		     int omp_num_threads=-1, 
		     int cpu_affinity_policy=-1, 
		     std::string writable_path="/data/local/tmp/");
	/**
	 * [destructor]
	 */
	virtual ~MaceRun();

	/**
	 * [run neural network model]
	 * run model 
	 * @param  inputs  {map} <name, data> pair
	 * @param  outputs {map} <name, data> pair
	 * @return         {boolean} true or false
	 */
	virtual bool run(std::map<std::string, unsigned char*> inputs, 
					 std::map<std::string, unsigned char*>& outputs);

	/**
	 * [initialize description]
	 * initialize mace model, only call once
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
	 * [parseDeviceType description]
	 * use device string to parse device type
	 * @param  device_str {string} "CPU","GPU",...
	 * @return            {DeviceType}
	 */
	mace::DeviceType parseDeviceType(const std::string device_str);

	std::string m_opencl_storage_path;
	std::shared_ptr<mace::MaceEngine> m_engine;
	std::map<std::string, mace::MaceTensor> m_inputs;
  	std::map<std::string, mace::MaceTensor> m_outputs;

    std::map<std::string, int64_t> m_inputs_size;

private:	
    bool m_is_ready;
};	
}

#include "mace_run.hpp"
#endif