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
#include "rknn_api.h"
#include "eagleeye/common/EagleeyeFile.h"


namespace eagleeye
{
template<typename Enabled>
class ModelRun<RknnRun, Enabled>:public ModelEngine{
public:
    typedef RknnRun                   ModelEngineType;

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
	virtual bool run(std::map<std::string, const unsigned char*> inputs, 
					 std::map<std::string, unsigned char*>& outputs);

    /**
	 * [initialize initialize running environment]
	 * @return {boolean} true or false
	 */
	virtual bool initialize();

    unsigned char* load_model(const char* filename, int* model_size);

private:

    rknn_context m_ctx;
	bool m_is_init;

    rknn_tensor_attr* m_input_attrs;
    rknn_tensor_attr* m_output_attrs;

    rknn_tensor_mem** m_input_mems;
    rknn_tensor_mem** m_output_mems;

    std::string m_model_name;
    rknn_input_output_num m_io_num;
};
} // namespace eagleeye

#include "eagleeye/engine/rknn_run.hpp"
#endif