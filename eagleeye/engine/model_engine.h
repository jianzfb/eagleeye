#ifndef _MODEL_ENGINE_H_
#define _MODEL_ENGINE_H_
#include <string>
#include <map>
#include <vector>

namespace eagleeye{
class ModelEngine{
public:
	ModelEngine(std::string model_name, 
				std::string device, 
				std::vector<std::string> input_names=std::vector<std::string>(),
		     	std::vector<std::vector<int64_t>> input_shapes=std::vector<std::vector<int64_t>>(),
		     	std::vector<std::string> output_names=std::vector<std::string>(),
		     	std::vector<std::vector<int64_t>> output_shapes=std::vector<std::vector<int64_t>>(),
		     	int omp_num_threads=-1,
		     	int cpu_affinity_policy=-1,
		     	std::string writable_path="/data/local/tmp/");

	virtual ~ModelEngine();
	/**
	 * [run description]
	 * @param  inputs  [description]
	 * @param  outputs [description]
	 * @return         [description]
	 */
	virtual bool run(std::map<std::string, unsigned char*> inputs, 
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
	 * @brief Get the Input Ptr object
	 * 
	 * @param input_name 
	 * @return void* 
	 */
	void* getInputPtr(std::string input_name){return NULL;};

protected:
	int m_omp_num_threads;
	int m_cpu_affinity_policy;
	std::string m_writable_path;

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
};
}

#endif