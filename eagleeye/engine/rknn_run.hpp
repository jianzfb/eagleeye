namespace eagleeye
{
template<typename Enabled>
ModelRun<RknnRun, Enabled>::ModelRun(std::string model_name, 
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
    this->m_model_name = model_name + ".rknn";
    this->m_is_init = false;
    this->m_ctx = 0;
}

template<typename Enabled>
ModelRun<RknnRun, Enabled>::~ModelRun(){
	// do nothing
    // Destroy rknn memory
    rknn_destroy_mem(m_ctx, this->m_input_mems[0]);
    for (uint32_t i = 0; i < this->m_io_num.n_output; ++i) {
        rknn_destroy_mem(this->m_ctx, this->m_output_mems[i]);
    }

    delete[] this->m_input_mems;
    delete[] this->m_output_mems;

    delete[] m_input_attrs;
    delete[] m_output_attrs;

    // destroy
    rknn_destroy(this->m_ctx);
}


template<typename Enabled>
bool ModelRun<RknnRun, Enabled>::run(std::map<std::string, unsigned char*> inputs, 
				   std::map<std::string, unsigned char*>& outputs){	
    // // ignore outside inputs
    // if(!this->m_predictor.get()){
    //     EAGLEEYE_LOGE("TNN predictor null.");
    //     return false;
    // }

    // // set input
    // for(int index=0; index < this->m_input_names.size(); ++index){
    //     // 输入节点名字
    //     std::string node_name = this->m_input_names[index];
	// 	if(inputs.find(node_name) == inputs.end()){
	// 		continue;
	// 	}

    //     // 输入节点形状
    //     std::vector<int64_t> node_shape = this->m_input_shapes[index];
    //     TNN_NS::DimsVector dims = {1,1,1,1};
    //     dims[0] = node_shape[0];
    //     dims[1] = node_shape[1];
    //     dims[2] = node_shape[2];
    //     dims[3] = node_shape[3];

    //     // 输入节点数据
    //     unsigned char* node_data = inputs[node_name];

    //     // 预处理参数 (仅支持3通道图像数据)
    //     if(node_shape[1] == 3 && (this->m_input_convert_params.find(node_name) != this->m_input_convert_params.end())){
    //         ConvertParam convert_param = this->m_input_convert_params[node_name];
    //         TNN_NS::MatConvertParam tnn_cvt_param;
    //         tnn_cvt_param.scale = convert_param.scale;
    //         tnn_cvt_param.bias = convert_param.bias;
    //         tnn_cvt_param.reverse_channel = convert_param.reverse_channel;

    //         // 设置模型输入
    //         auto frame_mat = 
    //             std::make_shared<TNN_NS::Mat>(TNN_NS::DEVICE_ARM, TNN_NS::N8UC3, dims, (uint8_t *) node_data);
    //         this->m_predictor->SetInputMat(frame_mat, tnn_cvt_param, node_name);
    //     }
    // }

    // // run model
    // this->m_predictor->Forward();
    
    // // get output
    // if(outputs.size() > 0){
	// 	std::map<std::string, unsigned char*>::iterator iter, iend(outputs.end());
	// 	for(iter = outputs.begin(); iter != iend; ++iter){
    //         std::shared_ptr<TNN_NS::Mat> tensor_mat = nullptr;
    //         this->m_predictor->GetOutputMat(tensor_mat,  TNN_NS::MatConvertParam(), iter->first);
	// 		iter->second = (unsigned char*)(tensor_mat->GetData());
	// 	}
    // }
    return true;
}

template<typename Enabled>
bool ModelRun<RknnRun, Enabled>::initialize(){
    if(this->m_is_init){
        EAGLEEYE_LOGD("RKNN Has been finished model initialize.");
        return true;
    }
    this->m_is_init = true;

    std::string model_folder = this->getModelFolder();
    if(model_folder == ""){
        EAGLEEYE_LOGD("RKNN Dont set model folder.");
        return false;
    }

    std::string model_path = model_folder + this->m_model_name;
    // Load RKNN Model
    int            model_len = 0;
    unsigned char* model     = load_model(model_path, &model_len);
    int            ret       = rknn_init(&this->m_ctx, model, model_len, 0, NULL);
    if (ret < 0) {
        EAGLEEYE_LOGD("rknn_init fail! ret=%d\n", ret);
        return false;
    }

    // Get sdk and driver version
    rknn_sdk_version sdk_ver;
    ret = rknn_query(this->m_ctx, RKNN_QUERY_SDK_VERSION, &sdk_ver, sizeof(sdk_ver));
    if (ret != RKNN_SUCC) {
        EAGLEEYE_LOGD("rknn_query fail! ret=%d\n", ret);
        return false;
    }
    EAGLEEYE_LOGD("rknn_api/rknnrt version: %s, driver version: %s\n", sdk_ver.api_version, sdk_ver.drv_version);

    // Get Model Input Output Info
    ret = rknn_query(m_ctx, RKNN_QUERY_IN_OUT_NUM, &m_io_num, sizeof(m_io_num));
    if (ret != RKNN_SUCC) {
        EAGLEEYE_LOGD("rknn_query fail! ret=%d\n", ret);
        return false;
    }
    EAGLEEYE_LOGD("model input num: %d, output num: %d\n", m_io_num.n_input, m_io_num.n_output);

    EAGLEEYE_LOGD("input tensors:\n");
    this->m_input_attrs = new rknn_tensor_attr[m_io_num.n_input];
    memset(this->m_input_attrs, 0, m_io_num.n_input * sizeof(rknn_tensor_attr));
    for (uint32_t i = 0; i < m_io_num.n_input; i++) {
        m_input_attrs[i].index = i;
        // query info
        ret = rknn_query(m_ctx, RKNN_QUERY_INPUT_ATTR, &(m_input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret < 0) {
            EAGLEEYE_LOGD("rknn_init error! ret=%d\n", ret);
            return false;
        }
        dump_tensor_attr(&m_input_attrs[i]);
    }

    EAGLEEYE_LOGD("output tensors:\n");
    this->m_output_attrs = new rknn_tensor_attr[m_io_num.n_output];
    memset(m_output_attrs, 0, m_io_num.n_output * sizeof(rknn_tensor_attr));
    for (uint32_t i = 0; i < m_io_num.n_output; i++) {
        m_output_attrs[i].index = i;
        // query info
        ret = rknn_query(m_ctx, RKNN_QUERY_OUTPUT_ATTR, &(m_output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            EAGLEEYE_LOGD("rknn_query fail! ret=%d\n", ret);
            return false;
        }
        dump_tensor_attr(&m_output_attrs[i]);
    }

    // Create input tensor memory
    this->m_input_mems = new rknn_tensor_mem*[1];
    // default input type is int8 (normalize and quantize need compute in outside)
    // if set uint8, will fuse normalize and quantize to npu
    this->m_input_attrs[0].type = RKNN_TENSOR_UINT8;
    // default fmt is NHWC, npu only support NHWC in zero copy mode
    this->m_input_attrs[0].fmt = RKNN_TENSOR_NHWC;
    m_input_mems[0] = rknn_create_mem(m_ctx, m_input_attrs[0].size_with_stride);

    // Create output tensor memory
    this->m_output_mems = new rknn_tensor_mem*[m_io_num.n_output];
    for (uint32_t i = 0; i < m_io_num.n_output; ++i) {
        // default output type is depend on model, this require float32 to compute top5
        // allocate float32 output tensor
        int output_size = output_attrs[i].n_elems * sizeof(float);
        m_output_mems[i]  = rknn_create_mem(m_ctx, output_size);
    }

    // Set input tensor memory
    ret = rknn_set_io_mem(m_ctx, m_input_mems[0], &m_input_attrs[0]);
    if (ret < 0) {
        EAGLEEYE_LOGD("rknn_set_io_mem fail! ret=%d\n", ret);
        return false;
    }

    // Set output tensor memory
    for (uint32_t i = 0; i < m_io_num.n_output; ++i) {
        // default output type is depend on model, this require float32 to compute top5
        output_attrs[i].type = RKNN_TENSOR_FLOAT32;
        // set output memory and attribute
        ret = rknn_set_io_mem(m_ctx, m_output_mems[i], &output_attrs[i]);
        if (ret < 0) {
            EAGLEEYE_LOGD("rknn_set_io_mem fail! ret=%d\n", ret);
            return false;
        }
    }

    free(model);
    return true;
}

template<typename Enabled>
unsigned char* ModelRun<RknnRun, Enabled>::load_model(const char* filename, int* model_size)
{
    FILE* fp = fopen(filename, "rb");
    if (fp == nullptr) {
        EAGLEEYE_LOGD("fopen %s fail!\n", filename);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    int            model_len = ftell(fp);
    unsigned char* model     = (unsigned char*)malloc(model_len);
    fseek(fp, 0, SEEK_SET);
    if (model_len != fread(model, 1, model_len, fp)) {
        EAGLEEYE_LOGD("fread %s fail!\n", filename);
        free(model);
        return NULL;
    }
    *model_size = model_len;
    if (fp) {
        fclose(fp);
    }
    return model;
}

} // namespace eagleeye
