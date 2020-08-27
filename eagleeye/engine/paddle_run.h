#ifndef _PADDLE_MODEL_RUN_H_
#define _PADDLE_MODEL_RUN_H_
#ifdef EAGLEEYE_PADDLE_SUPPORT
#include "eagleeye/engine/model_engine.h"
#include "paddle_api.h"  // NOLINT
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <memory> 
namespace eagleeye{
class ModelRun: public ModelEngine{
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
		     int omp_num_threads=-1, 
		     int cpu_affinity_policy=-1, 
		     std::string writable_path="/data/local/tmp/");

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
	 * @brief Get the Input Ptr object
	 * 
	 * @param input_name 
	 * @return void* 
	 */
	virtual void* getInputPtr(std::string input_name);

private:
    std::string m_nb_path;
    std::shared_ptr<paddle::lite_api::PaddlePredictor> m_predictor;
};
    
} // namespace eagleeye


#endif
#endif