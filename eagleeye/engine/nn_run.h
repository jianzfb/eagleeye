#ifndef _EAGLEEYE_MODELRUN_H_
#define _EAGLEEYE_MODELRUN_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/engine/model_engine.h"
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include<memory> 
namespace eagleeye{
class NNRun{
	/**
	 * 
	 */
	NNRun(std::string model_name, 
			 std::string device,
			 std::vector<std::string> input_names=std::vector<std::string>(),
		     std::vector<std::vector<int64_t>> input_shapes=std::vector<std::vector<int64_t>>(),
			 std::vector<std::string> output_names=std::vector<std::string>(),
		     std::vector<std::vector<int64_t>> output_shapes=std::vector<std::vector<int64_t>>(),
		     int omp_num_threads=-1, 
		     int cpu_affinity_policy=-1, 
		     std::string writable_path="/data/local/tmp/");

    virtual ~NNRun();

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

	/**
	 * @brief Get the Writable Path object
	 * 
	 * @return std::string 
	 */
	virtual std::string getWritablePath();

	/**
	 * [setOutputNameMap description]
	 * @param name_map [description]
	 */
	void setOutputNameMap(std::map<std::string, std::string> name_map);

	/**
	 * [setOutputNameMap2 description]
	 * @param name_map [description]
	 */
	void setOutputNameMap2(std::map<std::string, std::string> name_map);

	/**
	 * @brief Get the Input Ptr object
	 * 
	 * @param input_name 
	 * @return void* 
	 */
	void* getInputPtr(std::string input_name);

    /**
     * @brief get engein ptr
     * 
     * @return ModelEngine* 
     */
    ModelEngine* get();

private:
    ModelEngine* m_engine;
};    
}
#endif