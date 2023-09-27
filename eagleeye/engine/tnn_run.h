#ifndef _TNN_MODEL_RUN_H_
#define _TNN_MODEL_RUN_H_
#include "eagleeye/engine/model_engine.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/engine/model_run.h"
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <memory> 
#include <iostream>
#include "tnn/core/common.h"
#include "tnn/core/macro.h"
#include "tnn/core/tnn.h"
#include "tnn/utils/blob_converter.h"
#include "tnn/utils/cpu_utils.h"
#include "tnn/utils/mat_utils.h"
#include "eagleeye/common/EagleeyeFile.h"

namespace eagleeye
{
template<typename Enabled>
class ModelRun<TNNRun, Enabled>:public ModelEngine{
public:
    typedef TNNRun                   ModelEngineType;

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

	/**
	 * @brief refresh input
	 * 
	 */
	void refresh();

private:
    std::string m_tnnproto;
    std::string m_tnnmodel;
	std::shared_ptr<TNN_NS::Instance> m_predictor;

	std::map<std::string, std::shared_ptr<TNN_NS::Mat>> m_input_map;
	bool m_is_init;
	bool m_inner_preprocess;
};
} // namespace eagleeye

#include "eagleeye/engine/tnn_run.hpp"
#endif