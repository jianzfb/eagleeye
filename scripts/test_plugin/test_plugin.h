extern "C"{
    /**
     * @brief initialize test pipeline
     * 
     * @return true 
     * @return false 
     */
    bool eagleeye_test_initialize(const char* config_folder);

    /**
     * @brief load test pipeline configure
     * 
     * @param folder 
     * @return true 
     * @return false 
     */
    bool eagleeye_test_load_config(const char* config_file);

    /**
     * @brief save test pipeline configure
     * 
     * @param folder 
     * @return true 
     * @return false 
     */
    bool eagleeye_test_save_config(const char* config_file);

    /**
     * @brief release test pipeline
     * 
     * @return true 
     * @return false 
     */
    bool eagleeye_test_release();

    /**
     * @brief run test pipeline
     * 
     * @return true 
     * @return false 
     */
    bool eagleeye_test_run();

    /**
     * @brief get test pipeline version
     * 
     * @param pipeline_version
     * @return true 
     * @return false 
     */
    bool eagleeye_test_version(char* pipeline_version);

    /**
     * @brief reset test pipeline state
     * 
     * @return true 
     * @return false 
     */
    bool eagleeye_test_reset();

    /**
     * @brief set any node param in test pipeline
     * 
     * @param node_name node name in pipeline
     * @param param_name node param in pipeline
     * @param value param value
     * @return true  success to set
     * @return false fail to set
     */
    bool eagleeye_test_set_param(const char* node_name, const char* param_name, const void* value);

    /**
     * @brief get any node param in test pipeline
     * 
     * @param node_name node name in pipeline
     * @param param_name node param in pipeline
     * @param value param value
     * @return true success to get
     * @return false fail to get
     */
    bool eagleeye_test_get_param(const char* node_name, const char* param_name, void* value);

    /**
     * @brief set input data from test input node
     * 
     * @param node_name node name in pipeline
     * @param data data pointer
     * @param data_size dimension (H,W,C)
     * @param data_dims dimension number
     * @return true 
     * @return false 
     */
    bool eagleeye_test_set_input(const char* node_name, void* data, const int* data_size, const int data_dims);

    /**
     * @brief get output data from test output node
     * 
     * @param node_name node name in pipeline
     * @param data data pointer
     * @param data_size dimension (H,W,C)/(B,H,W,C)
     * @param data_dims dimension number
     * @param data_type data type
     * @return true 
     * @return false 
     */
    bool eagleeye_test_get_output(const char* node_name, void*& data, int* data_size, int& data_dims,int& data_type);
}