#ifndef _PADDLE_MODEL_RUN_H_
#define _PADDLE_MODEL_RUN_H_
#include "eagleeye/engine/model_engine.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/engine/model_run.h"
#include "eagleeye/common/EagleeyeFile.h"
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <memory> 
#include <iostream>
#include "paddle_api.h"  // NOLINT


namespace eagleeye{

template<typename Enabled>
class ModelRun<PaddleRun, Enabled>:public ModelEngine{
public:
	typedef PaddleRun                   ModelEngineType;

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
	 * @brief Get the Input Ptr object
	 * 
	 * @param input_name 
	 * @return void* 
	 */
	virtual void* getInputPtr(std::string input_name);

	/**
	 * @brief Get the Output Ptr object
	 * 
	 * @param input_name 
	 * @return void* 
	 */
	virtual const void* getOutputPtr(std::string output_name);

	/**
	 * @brief resize input tensor
	 * 
	 * @param input_name 
	 * @param shape 
	 */
	void resize(std::string input_name, std::vector<int64_t> shape);

	/**
	 * @brief Get the Input object
	 * 
	 * @param input_name 
	 * @param input_ptr 
	 * @param shape 
	 */
	void getInput(std::string input_name, void*& input_ptr, std::vector<int64_t>& shape);

	/**
	 * @brief Get the Output object
	 * 
	 * @param output_name 
	 * @param output_ptr 
	 * @param shape 
	 */
	void getOutput(std::string output_name, void*& output_ptr, std::vector<int64_t>& shape);

private:
    std::string m_nb_name;
    std::shared_ptr<paddle::lite_api::PaddlePredictor> m_predictor;
	bool m_is_init;
};
} // namespace eagleeye

#include "eagleeye/engine/paddle_run.hpp"
#endif