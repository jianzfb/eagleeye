using namespace nvinfer1;
namespace eagleeye{
template<typename Enabled>
ModelRun<TensorrtRun, Enabled>::ModelRun(std::string model_name, 
            std::string device,
            std::vector<std::string> input_names,
            std::vector<std::vector<int64_t>> input_shapes,
            std::vector<std::string> output_names,
            std::vector<std::vector<int64_t>> output_shapes,
			int num_threads, 
		    RunPower model_power, 
            std::string writable_path)
	:ModelEngine(model_name,
				 device,
				 input_names,
				 input_shapes,
				 output_names,
				 output_shapes,
				 num_threads,
				 model_power,
				 writable_path){
	this->m_model_name = model_name;
	if(!endswith(model_name, ".onnx")){
		this->m_model_name = this->m_model_name+".onnx";
	}    
	this->m_is_init = false;    
}

template<typename Enabled>
ModelRun<TensorrtRun, Enabled>::~ModelRun(){
    // Free the GPU memory
    for (auto & buffer : m_buffers) {
        checkCudaErrorCode(cudaFree(buffer));
    }

    m_buffers.clear();
}

template<typename Enabled>
bool ModelRun<TensorrtRun, Enabled>::run(
    std::map<std::string, const unsigned char*> inputs, 
    std::map<std::string, unsigned char*>& outputs){
    if(inputs.size() == 0){
        EAGLEEYE_LOGD("Provided input vector is empty!");
        return false;
    }

    const auto num_inputs = m_inputDims.size();
    if (inputs.size() != num_inputs) {
        EAGLEEYE_LOGD("Incorrect number of inputs provided!");
        return false;
    }

    // TODO，这里虽然动态设置，但是m_input_shapes参数是固定的。
    // 后期加入动态batch
    const auto batch_size = static_cast<int32_t>(this->m_input_shapes[0][0]);
    // Make sure the same batch size was provided for all inputs
    for (size_t i = 1; i < inputs.size(); ++i) {
        if (m_input_shapes[i][0] != static_cast<size_t>(batch_size)) {
            EAGLEEYE_LOGD("The batch size needs to be constant for all inputs!");
            return false;
        }
    }

    // Create the cuda stream that will be used for inference
    cudaStream_t inferenceCudaStream;
    checkCudaErrorCode(cudaStreamCreate(&inferenceCudaStream));

    // Preprocess all the inputs
    for (size_t i = 0; i < num_inputs; ++i) {
        // 节点名字
        const auto& dims = m_inputDims[i];
        std::string input_name = this->m_input_names[i];
        nvinfer1::Dims4 input_dims = {batch_size, dims.d[0], dims.d[1], dims.d[2]};

        // 重置大小
        m_context->setBindingDimensions(i, input_dims); // Define the batch size

        // 传递到CPU -> GPU
        checkCudaErrorCode(cudaMemcpyAsync(m_buffers[i], (const void*)(inputs[input_name]),
                                           input_dims.d[0] * input_dims.d[1] * input_dims.d[2] * input_dims.d[3] * sizeof(float),
                                           cudaMemcpyHostToDevice, inferenceCudaStream));
    }
    // Ensure all dynamic bindings have been defined.
    if (!m_context->allInputDimensionsSpecified()) {
        throw std::runtime_error("Error, not all required dimensions specified.");
    }

    // Run inference.
    bool status = m_context->enqueueV2(m_buffers.data(), inferenceCudaStream, nullptr);
    if (!status) {
        return false;
    }

    // Copy the outputs back to CPU
    std::vector<std::vector<float>> batchOutputs{};
    for (int32_t outputBinding = num_inputs; outputBinding < m_engine->getNbBindings(); ++outputBinding) {
        // We start at index m_inputDims.size() to account for the inputs in our m_buffers
        int local_output_i = outputBinding - num_inputs;
        auto outputLenFloat = m_outputLengthsFloat[local_output_i];
        if(m_featureVectors[local_output_i].size() != batch_size * outputLenFloat){
            m_featureVectors[local_output_i].resize(batch_size * outputLenFloat);
        }

        // Copy the output
        checkCudaErrorCode(cudaMemcpyAsync(m_featureVectors[local_output_i].data(), static_cast<char*>(m_buffers[outputBinding]), batch_size * outputLenFloat * sizeof(float), cudaMemcpyDeviceToHost, inferenceCudaStream));

        std::string node_name = this->m_engine_binds_to_name_map[outputBinding];
        outputs[node_name] = (unsigned char*)(m_featureVectors[local_output_i].data());
    }
    // Synchronize the cuda stream
    checkCudaErrorCode(cudaStreamSynchronize(inferenceCudaStream));
    checkCudaErrorCode(cudaStreamDestroy(inferenceCudaStream));
    return true;
}

template<typename Enabled>
void ModelRun<TensorrtRun, Enabled>::getDeviceNames(std::vector<std::string>& deviceNames) {
    int numGPUs;
    cudaGetDeviceCount(&numGPUs);

    for (int device=0; device<numGPUs; device++) {
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, device);

        deviceNames.push_back(std::string(prop.name));
    }
}

template<typename Enabled>
std::string ModelRun<TensorrtRun, Enabled>::serializeEngineOptions(const Options &options, const std::string& onnxModelPath) {
    const auto filenamePos = onnxModelPath.find_last_of('/') + 1;
    std::string engineName = onnxModelPath.substr(filenamePos, onnxModelPath.find_last_of('.') - filenamePos) + ".engine";
    if(endswith(this->m_writable_path, "/")){
        engineName = this->m_writable_path + engineName;
    }
    else{
        engineName = this->m_writable_path + std::string("/") + engineName;
    }

    // Add the GPU device name to the file to ensure that the model is only used on devices with the exact same GPU
    std::vector<std::string> deviceNames;
    getDeviceNames(deviceNames);

    if (static_cast<size_t>(options.deviceIndex) >= deviceNames.size()) {
        throw std::runtime_error("Error, provided device index is out of range!");
    }

    auto deviceName = deviceNames[options.deviceIndex];
    // Remove spaces from the device name
    deviceName.erase(std::remove_if(deviceName.begin(), deviceName.end(), ::isspace), deviceName.end());

    engineName+= "." + deviceName;
    // Serialize the specified options into the filename
    if (options.precision == Precision::FP16) {
        engineName += ".fp16";
    } else {
        engineName += ".fp32";
    }

    engineName += "." + std::to_string(options.maxBatchSize);
    engineName += "." + std::to_string(options.optBatchSize);
    engineName += "." + std::to_string(options.maxWorkspaceSize);

    return engineName;
}

template<typename Enabled>
bool ModelRun<TensorrtRun, Enabled>::doesFileExist(const std::string &filepath) {
    std::ifstream f(filepath.c_str());
    return f.good();
}


template<typename Enabled>
void ModelRun<TensorrtRun, Enabled>::checkCudaErrorCode(cudaError_t code) {
    if (code != 0) {
        std::string errMsg = "CUDA operation failed with code: " + std::to_string(code) + "(" + cudaGetErrorName(code) + "), with message: " + cudaGetErrorString(code);
        std::cout << errMsg << std::endl;
        throw std::runtime_error(errMsg);
    }
}


template<typename Enabled>
bool ModelRun<TensorrtRun, Enabled>::loadNetwork() {
    // Read the serialized model from disk
    std::ifstream file(m_engineName, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        throw std::runtime_error("Unable to read engine file");
    }

    m_runtime = std::unique_ptr<IRuntime>(createInferRuntime(m_logger));
    if (!m_runtime) {
        return false;
    }

    // Set the device index
    auto ret = cudaSetDevice(m_options.deviceIndex);
    if (ret != 0) {
        int numGPUs;
        cudaGetDeviceCount(&numGPUs);
        auto errMsg = "Unable to set GPU device index to: " + std::to_string(m_options.deviceIndex) +
                ". Note, your device has " + std::to_string(numGPUs) + " CUDA-capable GPU(s).";
        throw std::runtime_error(errMsg);
    }

    m_engine = std::unique_ptr<nvinfer1::ICudaEngine>(m_runtime->deserializeCudaEngine(buffer.data(), buffer.size()));
    if (!m_engine) {
        return false;
    }

    m_context = std::unique_ptr<nvinfer1::IExecutionContext>(m_engine->createExecutionContext());
    if (!m_context) {
        return false;
    }

    // Storage for holding the input and output buffers
    // This will be passed to TensorRT for inference
    m_buffers.resize(m_engine->getNbBindings());

    // Create a cuda stream
    cudaStream_t stream;
    checkCudaErrorCode(cudaStreamCreate(&stream));

    // Allocate GPU memory for input and output buffers
    m_outputLengthsFloat.clear();
    for (int i = 0; i < m_engine->getNbBindings(); ++i) {
        if (m_engine->bindingIsInput(i)) {
            // 输入信息， 并分配空间
            auto inputBindingDims = m_engine->getBindingDimensions(i);

            // Allocate memory for the input
            // Allocate enough to fit the max batch size (we could end up using less later)
            checkCudaErrorCode(cudaMallocAsync(&m_buffers[i], m_options.maxBatchSize * inputBindingDims.d[1] * inputBindingDims.d[2] * inputBindingDims.d[3] * sizeof(float), stream));

            // Store the input dims for later use
            m_inputDims.emplace_back(inputBindingDims.d[1], inputBindingDims.d[2], inputBindingDims.d[3]);
        } else {
            // 输出信息，并分配空间
            // The binding is an output
            uint32_t outputLenFloat = 1;
            auto outputDims = m_engine->getBindingDimensions(i);

            for (int j = 1; j < outputDims.nbDims; ++j) {
                // We ignore j = 0 because that is the batch size, and we will take that into account when sizing the buffer
                outputLenFloat *= outputDims.d[j];
            }

            m_outputLengthsFloat.push_back(outputLenFloat);
            m_engine_binds_to_name_map[i] = m_engine->getBindingName(i);
            // Now size the output buffer appropriately, taking into account the max possible batch size (although we could actually end up using less memory)
            checkCudaErrorCode(cudaMallocAsync(&m_buffers[i], outputLenFloat * m_options.maxBatchSize * sizeof(float), stream));
        }
    }

    // 设置模型预测后结果保存位置
    m_featureVectors.resize(m_outputLengthsFloat.size());

    // Synchronize and destroy the cuda stream
    checkCudaErrorCode(cudaStreamSynchronize(stream));
    checkCudaErrorCode(cudaStreamDestroy(stream));
    return true;
}


template<typename Enabled>
bool ModelRun<TensorrtRun, Enabled>::initialize(){
    if(this->m_is_init){
        EAGLEEYE_LOGD("Tensorrt Has been finished model initialize.");
        return true;
    }
    this->m_is_init = true;

    std::string model_folder = this->getModelFolder();
    if(model_folder == ""){
        EAGLEEYE_LOGD("tensorrt Dont set model folder.");
        return false;
    }
    std::string onnx_model_path = "";
    if(endswith(model_folder, "/")){
        onnx_model_path = model_folder + this->m_model_name;
    }
    else{
        onnx_model_path = model_folder + std::string("/") + this->m_model_name;
    }
    // 检查文件是否存在，否则更换查找位置
    if(!isfileexist(onnx_model_path.c_str())){
        std::string so_folder = this->getModelRoot();
        if(endswith(so_folder, "/")){
            onnx_model_path = so_folder + this->m_model_name;
        }
        else{
            onnx_model_path = so_folder + std::string("/") + this->m_model_name;
        }
    }

    EAGLEEYE_LOGD("Load TENSORRT model from %s", onnx_model_path.c_str());

    // Only regenerate the engine file if it has not already been generated for the specified options
    m_engineName = serializeEngineOptions(m_options, onnx_model_path);
    EAGLEEYE_LOGD("Searching for engine file with name: %s", m_engineName.c_str());

    if (doesFileExist(m_engineName)) {
        EAGLEEYE_LOGD("Engine found, not regenerating...");
        EAGLEEYE_LOGD("Directly load tensorrt engine.");
        bool ok = this->loadNetwork();
        return ok;
    }

    if (!doesFileExist(onnx_model_path)) {
        throw std::runtime_error("Could not find model at path: " + onnx_model_path);
    }

    // Was not able to find the engine file, generate...
    EAGLEEYE_LOGD("Engine not found, generating...");

    // Create our engine builder.
    auto builder = std::unique_ptr<nvinfer1::IBuilder>(nvinfer1::createInferBuilder(m_logger));
    if (!builder) {
        return false;
    }

    // Set the max supported batch size
    builder->setMaxBatchSize(m_options.maxBatchSize);

    // Define an explicit batch size and then create the network.
    // More info here: https://docs.nvidia.com/deeplearning/tensorrt/developer-guide/index.html#explicit-implicit-batch
    auto explicitBatch = 1U << static_cast<uint32_t>(NetworkDefinitionCreationFlag::kEXPLICIT_BATCH);
    auto network = std::unique_ptr<nvinfer1::INetworkDefinition>(builder->createNetworkV2(explicitBatch));
    if (!network) {
        return false;
    }

    // Create a parser for reading the onnx file.
    auto parser = std::unique_ptr<nvonnxparser::IParser>(nvonnxparser::createParser(*network, m_logger));
    if (!parser) {
        return false;
    }

    // We are going to first read the onnx file into memory, then pass that buffer to the parser.
    // Had our onnx model file been encrypted, this approach would allow us to first decrypt the buffer.

    std::ifstream file(onnx_model_path, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        throw std::runtime_error("Unable to read engine file");
    }

    auto parsed = parser->parse(buffer.data(), buffer.size());
    if (!parsed) {
        return false;
    }

    auto config = std::unique_ptr<nvinfer1::IBuilderConfig>(builder->createBuilderConfig());
    if (!config) {
        return false;
    }

    // Register a single optimization profile
    IOptimizationProfile *optProfile = builder->createOptimizationProfile();
    const int32_t num_inputs = network->getNbInputs();
    for (int32_t i = 0; i < num_inputs; ++i) {
        const auto input = network->getInput(i);
        const auto inputName = input->getName();
        const auto inputDims = input->getDimensions();
        int32_t inputC = inputDims.d[1];
        int32_t inputH = inputDims.d[2];
        int32_t inputW = inputDims.d[3];

        // Specify the optimization profile
        optProfile->setDimensions(inputName, OptProfileSelector::kMIN, Dims4(1, inputC, inputH, inputW));
        optProfile->setDimensions(inputName, OptProfileSelector::kOPT, Dims4(m_options.optBatchSize, inputC, inputH, inputW));
        optProfile->setDimensions(inputName, OptProfileSelector::kMAX, Dims4(m_options.maxBatchSize, inputC, inputH, inputW));
    }
    config->addOptimizationProfile(optProfile);
    config->setMaxWorkspaceSize(m_options.maxWorkspaceSize);

    if (m_options.precision == Precision::FP16) {
        config->setFlag(BuilderFlag::kFP16);
    }

    // CUDA stream used for profiling by the builder.
    cudaStream_t profileStream;
    checkCudaErrorCode(cudaStreamCreate(&profileStream));
    config->setProfileStream(profileStream);

    // Build the engine
    std::unique_ptr<IHostMemory> plan{builder->buildSerializedNetwork(*network, *config)};
    if (!plan) {
        return false;
    }

    // Write the engine to disk
    std::ofstream outfile(m_engineName, std::ofstream::binary);
    outfile.write(reinterpret_cast<const char*>(plan->data()), plan->size());

    EAGLEEYE_LOGD("Success, saved engine to %s", m_engineName.c_str());
    checkCudaErrorCode(cudaStreamDestroy(profileStream));  
    
    EAGLEEYE_LOGD("load tensorrt engine.");
    bool ok = this->loadNetwork();
    return ok;
}
} // namespace eagleeye