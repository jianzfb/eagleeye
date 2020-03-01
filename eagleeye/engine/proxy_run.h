#ifndef _PROXY_MODEL_RUN_H_
#define _PROXY_MODEL_RUN_H_
#include <string>
#include <map>
#include <vector>
#include "eagleeye/engine/model_engine.h"
#ifdef EAGLEEYE_MACE_SUPPORT
#include "eagleeye/engine/mace_run.h"
#elif EAGLEEYE_TF_SUPPORT
#include "eagleeye/engine/tf_run.h"
#elif EAGLEEYE_SNPE_SUPPORT
#include "eagleeye/engine/snpe_run.h"
#else
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
		     std::string writable_path="/data/local/tmp/")
	:ModelEngine(model_name,
				 device,
				 input_names,
				 input_shapes,
				 output_names,
				 output_shapes,
				 omp_num_threads,
				 cpu_affinity_policy,
				 writable_path){
		std::cout<<"hello"<<std::endl;
	}
	/**
	 * 
	 */
	virtual ~ModelRun(){}

	/**
	 * [run description]
	 * run model 
	 * @param  inputs  {map} <name, data> pair
	 * @param  outputs {map} <name, data> pair
	 * @return         {boolean} true or false
	 */
	virtual bool run(std::map<std::string, unsigned char*> inputs, 
					 std::map<std::string, unsigned char*>& outputs){}

	/**
	 * [initialize description]
	 * @return [description]
	 */
	virtual bool initialize(){};

protected:
	std::vector<std::string> m_input_names;
	std::vector<std::vector<int64_t>> m_input_shapes;
	std::vector<std::string> m_output_names;
	std::vector<std::vector<int64_t>> m_output_shapes;
};
}
#endif
#endif