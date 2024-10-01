#ifndef _MODEL_ENGINE_H_
#define _MODEL_ENGINE_H_
#include <string>
#include <map>
#include <vector>
#include <cstdint> 

namespace eagleeye{
enum RunPower{
    HIGH_POWER = 0,
    LOW_POWER,
    NO_BIND,
};

struct ConvertParam {
    std::vector<float> scale = {1.0f, 1.0f, 1.0f, 1.0f};
    std::vector<float> bias  = {0.0f, 0.0f, 0.0f, 0.0f};
    bool reverse_channel     = false;
};

class ModelEngine{
public:
	ModelEngine(std::string model_name, 
				std::string device, 
				std::vector<std::string> input_names=std::vector<std::string>(),
		     	std::vector<std::vector<int64_t>> input_shapes=std::vector<std::vector<int64_t>>(),
		     	std::vector<std::string> output_names=std::vector<std::string>(),
		     	std::vector<std::vector<int64_t>> output_shapes=std::vector<std::vector<int64_t>>(),
		     	int omp_num_threads=-1,
		     	RunPower model_power = HIGH_POWER,
		     	std::string writable_path="/data/local/tmp/");

	virtual ~ModelEngine();

	/**
	 * @brief Set/Get the Model Folder object
	 * 
	 * @param model_folder 
	 */
	virtual void setModelFolder(std::string model_folder);
	std::string getModelFolder();

	/**
	 * [run description]
	 * @param  inputs  [description]
	 * @param  outputs [description]
	 * @return         [description]
	 */
	virtual bool run(std::map<std::string, const unsigned char*> inputs, 
					 std::map<std::string, unsigned char*>& outputs){};

	/**
	 * [initialize description]
	 * @param fixed_shape 
	 * @return [description]
	 */
	virtual bool initialize(){};

	/**
	 * [isDynamicInputShape]
	 * @return [description]
	 */
	virtual bool isDynamicInputShape();

	/**
	 * [isDynamicOutputShape description]
	 * @return [description]
	 */
	virtual bool isDynamicOutputShape();
	
	/**
	 * [setInputShapes set input tensor shapes]
	 * @param input_shapes [description]
	 */
	void setInputShapes(std::vector<std::vector<int64_t>> input_shapes);

	/**
	 * @brief Set the Input Names object
	 * 
	 * @param input_names 
	 */
	void setInputNames(std::vector<std::string> input_names);

	/**
	 * @brief Get the Input Names object
	 * 
	 * @return std::vector<std::string> 
	 */
	std::vector<std::string> getInputNames();

	/**
	 * [setOutputShapes set output tensor shapes]
	 * @param output_shapes [description]
	 */
	void setOutputShapes(std::vector<std::vector<int64_t>> output_shapes);

	/**
	 * @brief Set the Output Names object
	 * 
	 * @param output_names 
	 */
	void setOutputNames(std::vector<std::string> output_names);

	/**
	 * @brief Get the Output Names object
	 * 
	 * @return std::vector<std::string> 
	 */
	std::vector<std::string> getOutputNames();

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
	 * [setInputNameMap description]
	 * @param name_map [description]
	 */
	void setInputNameMap(std::map<std::string, std::string> name_map);

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
	 * [getModelRoot description]
	 * @return [description]
	 */
	std::string getModelRoot();

	/**
	 * @brief convert bgr to Tensor(CHW)
	 * 
	 * @param src 
	 * @param output 
	 * @param width 
	 * @param height 
	 * @param means 
	 * @param scales 
	 */
	void bgrToTensorCHW(const uint8_t* src,
                       float* output,
                       int width,
                       int height,
                       float* means,
                       float* scales);

	/**
	 * @brief convert bgr to RGB Tensor(CHW)
	 * 
	 * @param src 
	 * @param output 
	 * @param width 
	 * @param height 
	 * @param means 
	 * @param scales 
	 */
	void bgrToRgbTensorCHW(const uint8_t* src,
						float* output,
						int width,
						int height,
						float* means,
						float* scales);

	/**
	 * @brief Set the Input Convert Param object(预处理)
	 * 
	 * @param node_name 
	 * @param convert_param 
	 */
	void setInputConvertParam(std::string node_name, ConvertParam convert_param){
		this->m_input_convert_params[node_name] = convert_param;
	}

	/**
	 * @brief Get the Input Ptr object
	 * 
	 * @param input_name 
	 * @return void* 
	 */
	virtual void* getInputPtr(std::string input_name){return NULL;};

	virtual void setDynamicBatchSize(int batch_size){m_dynamic_batch_size = batch_size;}
	virtual int getDynamicBatchSize(){return m_dynamic_batch_size;}

protected:
	int m_omp_num_threads;
	int m_cpu_affinity_policy;
	RunPower m_model_power;
	std::string m_writable_path;
	std::string m_model_folder;

	bool m_is_dynamic_input_shape;
	bool m_is_dynamic_output_shape;

	std::string m_model_name;
	std::string m_device;
	
	std::vector<std::string> m_input_names;
	std::vector<std::vector<int64_t>> m_input_shapes;
	std::vector<std::string> m_output_names;
	std::vector<std::vector<int64_t>> m_output_shapes;

	std::map<std::string, std::string> m_output_name_map;
	std::map<std::string, std::string> m_inv_output_name_map;

	std::map<std::string, std::string> m_output_name_map2;
	std::map<std::string, std::string> m_inv_output_name_map2;

	std::map<std::string, std::string> m_input_name_map;
	std::map<std::string, std::string> m_inv_input_name_map;

	std::string m_root;

	std::map<std::string, ConvertParam> m_input_convert_params;
	std::map<std::string, int> m_input_map_index;
	std::map<std::string, int> m_output_map_index;

	std::vector<float> m_mean;
	std::vector<float> m_std;

	int m_dynamic_batch_size;
};
}

#endif