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
	FIXED_PLACEHOLDER = 0,//0
	FIXED_CONV,//1
    FIXED_DEPTHWISE_CONV,//2
	FIXED_POOLING,//3
	FIXED_INNERPRODUCT,//4
	FIXED_BATCHNORM,//5
	FIXED_SOFTMAX,//6
	FIXED_CONCAT,//7
	FIXED_PRELU,//8
	FIXED_RELU,//9
	FIXED_SPLIT,//10
	FIXED_TILING,//11
	FIXED_SCALE,//12
	FIXED_ELTWISE,//13
	FIXED_DROPOUT,//14
    FIXED_RESIZE, //15
	RECOG_PERMUTE,
	RECOG_NORMALIZE,
	RECOG_PRIORBOX,
	RECOG_FLATTEN,
	FIXED_RESHAPE,
	RECOG_SSD_OUTPUT,
	FIXED_SHUFFLECHANNEL,
	FIXED_SLICE,
	FIXED_IDENTITY
};

class FixedCNNOp{
public:
    FixedCNNOp(int input_data_num, int output_data_num, int layer_flag, std::string op_name);
    virtual ~FixedCNNOp();

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
	
	/**
	 * @brief Get the Ouput object
	 * 
	 * @param data 
	 * @param fixed_data 
	 */
	virtual void getOuput(Tensor<float>& data, Tensor<FixedType>& fixed_data);

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