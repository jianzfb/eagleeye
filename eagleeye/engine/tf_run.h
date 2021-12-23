#ifndef _TF_MODEL_RUN_H_
#define _TF_MODEL_RUN_H_
#include "tensorflow/core/public/session.h"
#include "tensorflow/core/platform/env.h"
#include "eagleeye/engine/model_engine.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeStr.h"
#include <string>
#include <vector>
namespace eagleeye{
class TFRun:public ModelEngine{
public:
	/**
	 * 
	 */
	TFRun(std::string model_name, 
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
	virtual ~TFRun();

	/**
	 * [run description]
	 * run model 
	 * @param  inputs  {map} <name, data> pair
	 * @param  outputs {map} <name, data> pair
	 * @return         {boolean} true or false
	 */
	virtual bool run(std::map<std::string, unsigned char*> inputs, 
					 std::map<std::string, unsigned char*>& outputs);

	/**
	 * [initialize description]
	 * @param fixed_shape 
	 * @return [description]
	 */
	virtual bool initialize();

protected:
	tensorflow::Session* m_session;
	std::string m_graph_path;
	std::vector<tensorflow::Tensor> m_outputs;
};
}

#include "eagleeye/engine/tf_run.hpp"
#endif