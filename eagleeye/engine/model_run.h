#ifndef _MODEL_RUN_H_
#define _MODEL_RUN_H_
#include "eagleeye/engine/model_engine.h"


namespace eagleeye
{
class PaddleRun{
public:
	PaddleRun(){}
	~PaddleRun(){}
};

class TNNRun{
public:
    TNNRun(){}
    ~TNNRun(){}
};    

template<typename T, typename Enabled = void>
class ModelRun{
public:
    typedef T                       ModelEngineType;

    /**
     * @brief Construct a new Model Run object
     * 
     * @param model_name 
     * @param device 
     * @param input_names 
     * @param input_shapes 
     * @param output_names 
     * @param output_shapes 
     * @param num_threads 
     * @param model_power 
     * @param writable_path 
     */
    ModelRun(std::string model_name, 
			 std::string device,
			 std::vector<std::string> input_names=std::vector<std::string>(),
		     std::vector<std::vector<int64_t>> input_shapes=std::vector<std::vector<int64_t>>(),
			 std::vector<std::string> output_names=std::vector<std::string>(),
		     std::vector<std::vector<int64_t>> output_shapes=std::vector<std::vector<int64_t>>(),
		     int num_threads=-1, 
		     RunPower model_power = HIGH_POWER, 
		     std::string writable_path="/data/local/tmp/"){};

    virtual ~ModelRun(){};

    /**
	 * [run neural network model]
	 * @param  inputs  {map} <name, data> pair
	 * @param  outputs {map} <name, data> pair
	 * @return         {boolean} true or false
	 */
    virtual bool run(std::map<std::string, unsigned char*> inputs, 
					 std::map<std::string, unsigned char*>& outputs)=0;

    /**
	 * [initialize initialize running environment]
	 * @return {boolean} true or false
	 */
	virtual bool initialize() = 0;
};
} // namespace eagleeye


#endif