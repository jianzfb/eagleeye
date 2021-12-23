#ifndef _EAGLEEYE_FIXEDBASE_H_
#define _EAGLEEYE_FIXEDBASE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/util/quantization.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace nano{    

#define CNN_CONV_MAX_NORM_VALUE 127
#define CNN_CONV_DATA_NORM_VALUE 32768
#define CNN_CONV_DATA_NORM_MOVE 15


enum
{
	PLACEHOLDER = 0,        //0
	CONV,                   //1
    DEPTHWISE_CONV,         //2
	POOLING,                //3
	INNERPRODUCT,           //4
	BATCHNORM,              //5
	SOFTMAX,                //6
	CONCAT,                 //7
	PRELU,                  //8
	RELU,                   //9
	SPLIT,                  //10
	TILING,                 //11
	SCALE,                  //12
	ELTWISE,                //13
	DROPOUT,                //14
    RESIZE,                 //15
	RESHAPE,
	SHUFFLECHANNEL,
	SLICE,
	IDENTITY
};

class CNNOp{
public:
    CNNOp(int input_data_num, int output_data_num, int layer_flag, std::string op_name);
    virtual ~CNNOp();

	/**
     * @brief initialize Op
     * 
     * @param buf 
     * @param in_size 
     */
    virtual bool init(char *buf, int in_size){return true;};

    /**
     * @brief Set the Shape object
     * 
     * @param shape 
     * @return int 
     */
    virtual int setShape(std::vector<int64_t> shape) = 0;

    /**
     * @brief Get the Input Num object
     * 
     * @return int 
     */
    int getInputNum();

    /**
     * @brief Get the Input Shape object
     * 
     * @param index 
     * @return std::vector<int64_t> 
     */
    std::vector<int64_t> getInputShape(int index);

    /**
     * @brief Get the Output Num object
     * 
     * @return int 
     */
    int getOutputNum();

    /**
     * @brief Get the Output Shape object
     * 
     * @param index 
     * @return std::vector<int64_t> 
     */
    std::vector<int64_t> getOutputShape(int index);

	/**
	 * @brief Get the Layer Type object
	 * 
	 * @return int 
	 */
	int getLayerType(){
		return this->layer_flag_;
	}

	/**
	 * @brief Get the Layer Name object
	 * 
	 * @return std::string 
	 */
    std::string getLayerName();

	/**
	 * @brief Set the Input object
	 * 
	 * @param data 
	 */
	virtual void setInput(std::vector<Tensor<float>>& data){};
	
	// /**
	//  * @brief Get the Ouput object
	//  * 
	//  * @param data 
	//  * @param fixed_data 
	//  */
	// virtual void getOuput(Tensor<float>& data, Tensor<FixedType>& fixed_data);

    /**
     * @brief run op on CPU
     * 
     */
    virtual void runOnCPU(){};

    /**
     * @brief run op on GPU
     * 
     */
    virtual void runOnGPU(){};

protected:
    /**
     * @brief expand data in 8bit
     * 
     * @param dst_data 
     * @param src_data 
     * @param channel 
     * @param src_wd 
     * @param src_ht 
     * @param pad_w_left 
     * @param pad_h_top 
     * @param pad_w_right 
     * @param pad_h_bottom 
     * @param const_value 
     */
    void MakeExpandData_8Bit(FixedConvType *dst_data, FixedConvType *src_data, int channel, int src_wd, int src_ht, int pad_w_left, int pad_h_top, int pad_w_right, int pad_h_bottom, int const_value);

	float* m_output_multi_rate; //fixed data multiple rate by channel
    float* m_input_multi_rate;  //fixed data multiple rate by channel

    std::vector<std::vector<int64_t>> m_output_shape;
    std::vector<std::vector<int64_t>> m_input_shape;
    std::string m_op_name;
    int layer_flag_;
    int input_data_num_, output_data_num_;
    int model_size_;
};
}
}
#endif