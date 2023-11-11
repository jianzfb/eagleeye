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
		     std::string writable_path,
             bool inner_preprocess)
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
    this->m_ctx = -1;
    this->m_inner_preprocess = inner_preprocess;
}

template<typename Enabled>
ModelRun<RknnRun, Enabled>::~ModelRun(){
	// do nothing
    // Destroy rknn memory
    for(uint32_t i = 0; i < this->m_io_num.n_input; ++i){
        rknn_destroy_mem(this->m_ctx, this->m_input_mems[i]);
    }
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
bool ModelRun<RknnRun, Enabled>::run(std::map<std::string, const unsigned char*> inputs, 
				   std::map<std::string, unsigned char*>& outputs){	
    // ignore outside inputs
    if(this->m_ctx < 0){
        EAGLEEYE_LOGE("RKNN null.");
        return false;
    }

    // set input
    for(int index=0; index < this->m_input_names.size(); ++index){
        // 输入节点名字
        std::string node_name = this->m_input_names[index];
		if(inputs.find(node_name) == inputs.end()){
			continue;
		}

        const unsigned char* input_data = inputs[node_name];
        if(this->m_inner_preprocess){
            // 输入数据为NHWC IMAGE TENSOR
            int width  = this->m_input_attrs[index].dims[2];
            int stride = m_input_attrs[index].w_stride;
            if (width == stride) {
                memcpy(this->m_input_mems[index]->virt_addr, input_data, width * m_input_attrs[index].dims[1] * m_input_attrs[index].dims[3]);
            } else {
                int height  = m_input_attrs[index].dims[1];
                int channel = m_input_attrs[index].dims[3];
                // copy from src to dst with stride
                const uint8_t* src_ptr = input_data;
                uint8_t* dst_ptr = (uint8_t*)this->m_input_mems[index]->virt_addr;
                // width-channel elements
                int src_wc_elems = width * channel;
                int dst_wc_elems = stride * channel;
                for (int h = 0; h < height; ++h) {
                    memcpy(dst_ptr, src_ptr, src_wc_elems);
                    src_ptr += src_wc_elems;
                    dst_ptr += dst_wc_elems;
                }
            }
        }
        else{
            // 输入数据为NCHW FLOAT TENSOR
            memcpy(this->m_input_mems[index]->virt_addr, input_data, m_input_attrs[index].dims[1]*m_input_attrs[index].dims[2]*m_input_attrs[index].dims[3]*sizeof(float));
        }
    }

    // run model
    rknn_run(this->m_ctx, NULL);

    // get output
    if(outputs.size() > 0){
        for(int index=0; index<this->m_output_names.size(); ++index){
            std::string node_name = this->m_output_names[index];
            if(outputs.find(node_name) == outputs.end()){
                continue;
            }

            outputs[node_name] = (unsigned char*)(this->m_output_mems[index]->virt_addr);
        }
    }
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

    std::string model_path = "";
    if(endswith(model_folder, "/")){
        model_path = model_folder + this->m_model_name;
    }
    else{
        model_path = model_folder + std::string("/") + this->m_model_name;
    }

    EAGLEEYE_LOGD("Try Load RKNN model from %s", model_path.c_str());

    // 检查文件是否存在，否则更换查找位置
    if(!isfileexist(model_path.c_str())){
        std::string so_folder = this->getModelRoot();
        if(endswith(so_folder, "/")){
            model_path = so_folder + this->m_model_name;
        }
        else{
            model_path = so_folder + std::string("/") + this->m_model_name;
        }
    }

    EAGLEEYE_LOGD("Final Load RKNN model from %s", model_path.c_str());

    // Load RKNN Model
    int            model_len = 0;
    unsigned char* model     = load_model(model_path.c_str(), &model_len);
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
        // dump_tensor_attr(&m_input_attrs[i]);
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
        // dump_tensor_attr(&m_output_attrs[i]);
    }

    // Create input tensor memory
    this->m_input_mems = new rknn_tensor_mem*[m_io_num.n_input];
    for(uint32_t i = 0; i < m_io_num.n_input; ++i){
        if(m_inner_preprocess){
            // default input type is int8 (normalize and quantize need compute in outside)
            // if set uint8, will fuse normalize and quantize to npu
            this->m_input_attrs[i].type = RKNN_TENSOR_UINT8;
            // default fmt is NHWC, npu only support NHWC in zero copy mode
            this->m_input_attrs[i].fmt = RKNN_TENSOR_NHWC;
            m_input_mems[i] = rknn_create_mem(m_ctx, m_input_attrs[i].size_with_stride);
        }
        else{
            this->m_input_attrs[i].type = RKNN_TENSOR_FLOAT32;
            this->m_input_attrs[i].fmt = RKNN_TENSOR_NCHW;
            m_input_mems[i] = rknn_create_mem(m_ctx, m_input_attrs[i].n_elems * sizeof(float));
        }
    }

    // Create output tensor memory
    this->m_output_mems = new rknn_tensor_mem*[m_io_num.n_output];
    for (uint32_t i = 0; i < m_io_num.n_output; ++i) {
        // default output type is depend on model, this require float32 to compute top5
        // allocate float32 output tensor
        int output_size = m_output_attrs[i].n_elems * sizeof(float);
        m_output_mems[i]  = rknn_create_mem(m_ctx, output_size);
    }

    // Set input tensor memory
    for(uint32_t i = 0; i < m_io_num.n_input; ++i){
        ret = rknn_set_io_mem(m_ctx, m_input_mems[i], &m_input_attrs[i]);
        if (ret < 0) {
            EAGLEEYE_LOGD("input %d, rknn_set_io_mem fail! ret=%d\n", i, ret);
            return false;
        }
    }

    // Set output tensor memory
    for (uint32_t i = 0; i < m_io_num.n_output; ++i) {
        // default output type is depend on model, this require float32 to compute top5
        m_output_attrs[i].type = RKNN_TENSOR_FLOAT32;
        // set output memory and attribute
        ret = rknn_set_io_mem(m_ctx, m_output_mems[i], &m_output_attrs[i]);
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
