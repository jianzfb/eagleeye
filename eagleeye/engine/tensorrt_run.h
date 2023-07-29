#ifndef _EAGLEEYE_TENSORRT_RUN_H_
#define _EAGLEEYE_TENSORRT_RUN_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/engine/model_engine.h"
#include "eagleeye/engine/model_run.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeTime.h"
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <memory> 
#include "NvInfer.h"
#include "NvOnnxParser.h"
#include "eagleeye/common/EagleeyeFile.h"

namespace eagleeye{

// Precision used for GPU inference
enum class Precision {
    FP32,
    FP16
};

// Options for the network
struct Options {
    bool doesSupportDynamicBatchSize = false;
    // Precision to use for GPU inference. 16 bit is faster but may reduce accuracy.
    Precision precision = Precision::FP16;
    // The batch size which should be optimized for.
    int32_t optBatchSize = 1;
    // Maximum allowable batch size
    int32_t maxBatchSize = 1;
    // Max allowable GPU memory to be used for model conversion, in bytes.
    // Applications should allow the engine builder as much workspace as they can afford;
    // at runtime, the SDK allocates no more than this and typically less.
    size_t maxWorkspaceSize = 4000000000;
    // GPU device index
    int deviceIndex = 0;
};

// Class to extend TensorRT logger
class NvInferLoggerC : public nvinfer1::ILogger {
    void log (Severity severity, const char* msg) noexcept override{
    // Would advise using a proper logging utility such as https://github.com/gabime/spdlog
    // For the sake of this tutorial, will just log to the console.

    std::cout << msg << std::endl;
}
};

template<typename Enabled>
class ModelRun<TensorrtRun, Enabled>: public ModelEngine{
public:
	ModelRun(std::string model_name, 
			 std::string device,
			 std::vector<std::string> input_names=std::vector<std::string>(),
		     std::vector<std::vector<int64_t>> input_shapes=std::vector<std::vector<int64_t>>(),
			 std::vector<std::string> output_names=std::vector<std::string>(),
		     std::vector<std::vector<int64_t>> output_shapes=std::vector<std::vector<int64_t>>(),
		     int num_threads = -1, 
		     RunPower model_power = HIGH_POWER, 
		     std::string writable_path="/data/local/tmp/");

    virtual ~ModelRun();

    /**
	 * [initialize initialize running environment]
	 * @return {boolean} true or false
	 */
	virtual bool initialize();

    /**
	 * [run neural network model]
	 * @param  inputs  {map} <name, data> pair
	 * @param  outputs {map} <name, data> pair
	 * @return         {boolean} true or false
	 */
	virtual bool run(std::map<std::string, const unsigned char*> inputs, 
					 std::map<std::string, unsigned char*>& outputs);

protected:
    // Converts the engine options into a string
    std::string serializeEngineOptions(const Options& options, const std::string& onnxModelPath);

    void getDeviceNames(std::vector<std::string>& deviceNames);

    bool doesFileExist(const std::string& filepath);

    inline void checkCudaErrorCode(cudaError_t code);

    bool loadNetwork();

    // Holds pointers to the input and output GPU buffers
    std::vector<void*> m_buffers;
    std::vector<uint32_t> m_outputLengthsFloat{};
    std::vector<nvinfer1::Dims3> m_inputDims;

	std::vector<std::vector<float>> m_featureVectors;

    std::unique_ptr<nvinfer1::IRuntime> m_runtime = nullptr;
    std::unique_ptr<nvinfer1::ICudaEngine> m_engine = nullptr;
    std::unique_ptr<nvinfer1::IExecutionContext> m_context = nullptr;
    Options m_options;
    NvInferLoggerC m_logger;
    std::string m_engineName;

    std::map<int32_t, std::string> m_engine_binds_to_name_map;
private:
	bool m_is_init;
};
} // namespace eagleeye


#include "eagleeye/engine/tensorrt_run.hpp"
#endif