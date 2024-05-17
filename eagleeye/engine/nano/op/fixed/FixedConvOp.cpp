#include "eagleeye/engine/nano/op/FixedConvOp.h"
#if defined (__ARM_NEON) || defined (__ARM_NEON__)
#include <arm_neon.h>
#endif

namespace eagleeye
{
namespace nano{	
FixedConvOp::FixedConvOp(int input_data_num, int output_data_num, std::string op_name):
             FixedCNNOp(input_data_num, output_data_num, FIXED_CONV, op_name){
	fixed_weight_ = NULL;
	fixed_bias_value_ = NULL;
	weight_group_rate_ = NULL;
	dilation_ = 1;
	output_data_num_ = output_data_num;
	input_data_num_ = input_data_num;
}  

FixedConvOp::~FixedConvOp(){
	EAGLEEYE_SAFEFREE(weight_group_rate_);
// #ifdef CNN_USING_OPENCL_GPU_PROCESSING
// 	if(new_weight_cl)
// 		clReleaseMemObject(new_weight_cl);
//     new_weight_cl = NULL;
// 	if(new_bias_value_cl)
// 		clReleaseMemObject(new_bias_value_cl);
//     new_bias_value_cl = NULL;
// #endif
}

void FixedConvOp::run_on_cpu(std::vector<Tensor<FixedType>>& output_data, 
                             std::vector<Tensor<FixedType>>& input_data){
	std::vector<int64_t> input_shape = input_data[0].shape();
	int input_data_size = input_shape[0]*input_shape[1]*input_shape[2]*input_shape[3];
	std::vector<int64_t> output_shape = output_data[0].shape();
	int output_data_size = output_shape[0]*output_shape[1]*output_shape[2]*output_shape[3];

	// Batch x Channel x Height x Weight
	int input_channel = input_shape[1];
	int input_height = input_shape[2];
	int input_width = input_shape[3];
	int output_height = output_shape[2];
	int output_width = output_shape[3];
	int output_channel = output_shape[1];
	int input_offset_ = input_data_size / group_;
	int output_offset_ = output_data_size / group_;

	int kernel_dim_ = kernel_size_*kernel_size_*input_shape[1] / group_;
	int weight_offset_ = output_shape[1] * kernel_dim_ / group_;
	int bias_offset_ = input_channel / group_;

    FixedType* net_input_data = (FixedType*)input_data[0].cpu();
    FixedType* net_output_data = (FixedType*)output_data[0].cpu();

    for (int g = 0; g < group_; ++g){
		bool bRet = Forward_SingleGroup(net_input_data + input_offset_*g, 
                                        net_output_data + output_offset_*g, 
                                        fixed_weight_ + g*weight_offset_,
                                        weight_group_rate_ + g*output_channel / group_,
				                        fixed_bias_value_ + g*bias_offset_, 
                                        input_channel / group_, 
                                        input_width, 
                                        input_height, 
                                        output_channel / group_, 
                                        output_width, 
                                        output_height);
		if (!bRet){
            EAGLEEYE_LOGE("error at op %s", this->m_op_name.c_str());
            return;
        }
	}
}

void FixedConvOp::run_on_gpu(std::vector<Tensor<FixedType>>& output, 
                             std::vector<Tensor<FixedType>>& input){

}

void FixedConvOp::run_on_dsp(std::vector<Tensor<FixedType>>& output, 
                             std::vector<Tensor<FixedType>>& input){

}

bool FixedConvOp::Forward_SingleGroup(FixedType *in_data, 
											FixedType *out_data, 
											FixedConvType *weight_data, 
											float *weight_rate,
										  	FixedBiasType *fixed_bias, 
											int input_channel, 
											int input_width,
											int input_height, 
											int output_channel, 
											int output_width, 
											int output_height)
{
	int i = 0, j = 0;
	FixedConvType *new_input_data = NULL;
	FixedConvType *expand_input_data = NULL;	//
	int expand_width = input_width + 2 * pad_size_;
	int expand_height = input_height + 2 * pad_size_;
	int kernel_size_sq = kernel_size_*kernel_size_;
	int input_size = input_channel*input_width*input_height;
	int *adjust_div_value = NULL; 

    //int64_t t1 = getTimeNsec();

	expand_input_data = (FixedConvType *)malloc(sizeof(FixedConvType)*expand_width*expand_height*input_channel);
	new_input_data = (FixedConvType *)malloc(sizeof(FixedConvType)*input_size);
	adjust_div_value = (int *)malloc(sizeof(int)*output_channel);
	if (expand_input_data == NULL || new_input_data == NULL || adjust_div_value == NULL)
	{
		EAGLEEYE_SAFEFREE(expand_input_data);
		EAGLEEYE_SAFEFREE(new_input_data);
		EAGLEEYE_SAFEFREE(adjust_div_value);
		return false;
	}
	const int max_norm_value = CNN_CONV_MAX_NORM_VALUE;
    const int data_norm_value = CNN_CONV_DATA_NORM_VALUE;
    const int data_norm_move = CNN_CONV_DATA_NORM_MOVE;
    const int norm_half_add_value = CNN_CONV_DATA_NORM_VALUE/2;

#if defined (__ARM_NEON) || defined (__ARM_NEON__)
    int max_abs_value = 0;
	int search_start = 0;
	int search_end = input_size;
	int search_8_end = (search_end - search_start) / 8 * 8 + search_start;
	short *ptr = in_data;
	signed char *ptr_new = new_input_data;
	short temp_save_value[8];
    int16x8_t in_neon, max_neon;

    //get max value of input data
    max_neon = vdupq_n_s16(0);
    for (i = search_start; i < search_8_end; i += 8)
    {
        in_neon = vld1q_s16(ptr);
        in_neon = vabsq_s16(in_neon);
        max_neon = vmaxq_s16(max_neon, in_neon);
        ptr += 8;
    }
    vst1q_s16(temp_save_value, max_neon);
    for (j = 0; j < 8; ++j)
    {
        if (temp_save_value[j] > max_abs_value)
            max_abs_value = temp_save_value[j];
    }
    for (; i < search_end; ++i)
    {
        int abs_value = abs(in_data[i]);
        if (abs_value > max_abs_value)
            max_abs_value = abs_value;
    }

	//归一化，并记下rate
	//(采用对称量化，在tensorflow量化训练时应该采用对应方法) @zhangjian
	// 在整个代码中采用两种不同的量化方式，
	// 1.step 在算子之间，转换到short类型
	// 2.step 在进入算子时，进行对称量化
	// 3.step 在执行完计算后，转换回short类型
	float input_adjust_rate = float(max_norm_value) / max_abs_value;
	int adjust_multi_value = int(input_adjust_rate*data_norm_value + 0.5);
	for (j = 0; j < output_channel; ++j)
	{
		if (weight_rate[j] == 0)
			adjust_div_value[j] = 0;
		else
			adjust_div_value[j] = int(data_norm_value / (input_adjust_rate*weight_rate[j]) + 0.5);
	}

    if(adjust_multi_value < data_norm_value)
    {
        int32x4_t mul_neon;
        int32x4_t half_value_neon = vdupq_n_s32(norm_half_add_value);
        int16x8_t in_neon, out_neon;
        int8x8_t out_neon_s8;
        int16x4_t in_neon_0, in_neon_1;
        int16x4_t out_neon_0, out_neon_1;
        ptr = in_data;
        ptr_new = new_input_data;
        for (i = search_start; i < search_8_end; i += 8)
        {
            in_neon = vld1q_s16(ptr);
            in_neon_0 = vget_low_s16(in_neon);
            in_neon_1 = vget_high_s16(in_neon);
            mul_neon = vmull_n_s16(in_neon_0, adjust_multi_value);
            mul_neon = vaddq_s32(mul_neon, half_value_neon);
            out_neon_0 = vshrn_n_s32(mul_neon, data_norm_move);
            mul_neon = vmull_n_s16(in_neon_1, adjust_multi_value);
            mul_neon = vaddq_s32(mul_neon, half_value_neon);
            out_neon_1 = vshrn_n_s32(mul_neon, data_norm_move);
            out_neon = vcombine_s16(out_neon_0, out_neon_1);
            out_neon_s8 = vmovn_s16(out_neon);
            vst1_s8(ptr_new, out_neon_s8);
            ptr += 8;
            ptr_new += 8;
        }
        if (i < search_end&&search_end - search_start >= 8)
        {
            i = search_end - 8;
            in_neon = vld1q_s16(in_data + i);
            in_neon_0 = vget_low_s16(in_neon);
            in_neon_1 = vget_high_s16(in_neon);
            mul_neon = vmull_n_s16(in_neon_0, adjust_multi_value);
            mul_neon = vaddq_s32(mul_neon, half_value_neon);
            out_neon_0 = vshrn_n_s32(mul_neon, data_norm_move);
            mul_neon = vmull_n_s16(in_neon_1, adjust_multi_value);
            mul_neon = vaddq_s32(mul_neon, half_value_neon);
            out_neon_1 = vshrn_n_s32(mul_neon, data_norm_move);
            out_neon = vcombine_s16(out_neon_0, out_neon_1);
            out_neon_s8 = vmovn_s16(out_neon);
            vst1_s8(new_input_data + i, out_neon_s8);
        }
        else {
            for (; i < search_end; ++i)
            {
                new_input_data[i] = ((in_data[i] * adjust_multi_value + norm_half_add_value) >> data_norm_move);
            }
        }
    }
    else
    {
        for (i = 0; i < search_end; ++i)
        {
            new_input_data[i] = ((in_data[i] * adjust_multi_value + norm_half_add_value) >> data_norm_move);
        }
    }

#else
#ifdef EAGLEEYE_SSE_OPTIMIZATION
	int max_abs_value = 0;
	int search_start = 0;
	int search_end = input_size;
	int search_8_end = (search_end - search_start) / 8 * 8 + search_start;
	int search_4_end = (search_end - search_start) / 4 * 4 + search_start;// -4;
	short *ptr = in_data;
	signed char *ptr_new = new_input_data;
	short *temp_save_value;
	__m128i in_sse, max_sse;

	//get max value of input data
	max_sse = _mm_set1_epi16(0);
	for (i = search_start; i < search_8_end; i += 8)
	{
		in_sse = _mm_loadu_si128((__m128i *)(ptr));
		in_sse = _mm_abs_epi16(in_sse);
		max_sse = _mm_max_epi16(max_sse, in_sse);
		ptr += 8;
	}
	temp_save_value = (short*)&max_sse;

	for (j = 0; j < 8; ++j)
	{
		if (temp_save_value[j] > max_abs_value)
			max_abs_value = temp_save_value[j];
	}
	for (; i < search_end; ++i)
	{
		int abs_value = abs(in_data[i]);
		if (abs_value > max_abs_value)
			max_abs_value = abs_value;
	}

	//归一化，并记下rate
	float input_adjust_rate = float(max_norm_value) / max_abs_value;
	int adjust_multi_value = int(input_adjust_rate*data_norm_value + 0.5);
	for (j = 0; j < output_channel; ++j)
	{
		if (weight_rate[j] == 0)
			adjust_div_value[j] = 0;
		else
			adjust_div_value[j] = int(data_norm_value / (input_adjust_rate*weight_rate[j]) + 0.5);
	}

	__m128i half_value_sse = _mm_set1_epi32(norm_half_add_value);
	__m128i adjust_multi_value_sse = _mm_set1_epi32(adjust_multi_value);
	ptr = in_data;
	for (i = search_start; i < search_4_end; i += 4)
	{
		in_sse = _mm_loadu_si128((__m128i *)(ptr));
		in_sse = _mm_cvtepi16_epi32(in_sse);
		in_sse = _mm_mullo_epi32(in_sse, adjust_multi_value_sse);
		in_sse = _mm_add_epi32(in_sse, half_value_sse);
		in_sse = _mm_srai_epi32(in_sse, data_norm_move);
		*ptr_new = _mm_extract_epi32(in_sse, 0); ptr_new++;
		*ptr_new = _mm_extract_epi32(in_sse, 1); ptr_new++;
		*ptr_new = _mm_extract_epi32(in_sse, 2); ptr_new++;
		*ptr_new = _mm_extract_epi32(in_sse, 3); ptr_new++;
		ptr += 4;
	}

	for (; i < search_end; ++i)
	{
		new_input_data[i] = ((in_data[i] * adjust_multi_value + norm_half_add_value) >> data_norm_move);
	}
#else
	int max_abs_value = 0;

	max_abs_value = 0;
	for (j = 0; j < input_size; ++j)
	{
		int abs_value = abs(in_data[j]);
		if (abs_value > max_abs_value)
			max_abs_value = abs_value;
	}
	//归一化，并记下rate
	float input_adjust_rate = float(max_norm_value) / max_abs_value;
	int adjust_multi_value = int(input_adjust_rate*data_norm_value + 0.5);

	for (j = 0; j < output_channel; ++j)
	{
		if (weight_rate[j] == 0)
			adjust_div_value[j] = 0;
		else
			adjust_div_value[j] = int(data_norm_value / (input_adjust_rate*weight_rate[j]) + 0.5);
	}

	for (j = 0; j < input_size; ++j)
	{
		new_input_data[j] = ((in_data[j] * adjust_multi_value + norm_half_add_value) >> data_norm_move);
	}
#endif
#endif

	if (expand_width == input_width&&expand_height == input_height)
	{
		memcpy(expand_input_data, new_input_data, sizeof(FixedConvType)*expand_width*expand_height*input_channel);
	}
	else
	{
		MakeExpandData_8Bit(expand_input_data, new_input_data, input_channel, input_width, input_height, pad_size_, pad_size_, pad_size_, pad_size_, 0);//外扩，外扩部分设为常数0
	}

	int image_size = output_width*output_height;
	int total_kernel_size = kernel_size_sq*input_channel;
	bool bRet = false;
  //  LOGI1("%d %d %d, total: %d", output_channel, expand_width*output_height, total_kernel_size, output_channel*expand_width*output_height*total_kernel_size);

  //  int64_t t1 = getTimeNsec();
//   for(int ii = 0;ii < 1000; ++ii)
	{
		// if (w_stride_ > 2|| image_size < total_kernel_size / 2)
		// {
	////	   LOGI1("step1");
			bRet = RunConvForward_KernelLoop(expand_input_data, input_channel, expand_width, expand_height, weight_data, out_data, output_channel,\
											 output_width, output_height, kernel_size_, pad_size_, w_stride_, h_stride_, fixed_bias, adjust_div_value, norm_half_add_value, bool_16Bit_32Bit_Add_);
		// }
		// else
		// {
        // //    LOGI1("step2");
		// 	bRet = RunConvForward_ImageLoop(expand_input_data, input_channel, expand_width, expand_height, weight_data, out_data, output_channel,\
		// 									output_width, output_height, kernel_size_, pad_size_, w_stride_, h_stride_, fixed_bias, adjust_div_value, norm_half_add_value, bool_16Bit_32Bit_Add_);
		// }
	}
  //  int64_t t2 = getTimeNsec();
   //	LOGI1("conv: %d %fms", layer_index, (t2 - t1)/1000000.);
	EAGLEEYE_SAFEFREE(adjust_div_value);
	EAGLEEYE_SAFEFREE(expand_input_data);
	EAGLEEYE_SAFEFREE(new_input_data);
	return bRet;
}

bool FixedConvOp::init(char *buf, int in_size){
    	bool bSuc = true;
	int size = 0;
	char *ptr = buf;
	char temp_buf[4];
	int input_channel, output_channel;

	memcpy(temp_buf, ptr, sizeof(int));
	input_channel = *((int *)temp_buf);
	ptr += sizeof(int);

	memcpy(temp_buf, ptr, sizeof(int));
	output_channel = *((int *)temp_buf);
	ptr += sizeof(int);

	memcpy(temp_buf, ptr, sizeof(int));
	kernel_size_ = *((int *)temp_buf);
	ptr += sizeof(int);

	memcpy(temp_buf, ptr, sizeof(int));
	dilation_ = *((int *)temp_buf);
	ptr += sizeof(int);

	memcpy(temp_buf, ptr, sizeof(int));
	pad_size_ = *((int *)temp_buf);
	ptr += sizeof(int);

	memcpy(temp_buf, ptr, sizeof(int));
	w_stride_ = *((int *)temp_buf);
	ptr += sizeof(int);

	memcpy(temp_buf, ptr, sizeof(int));
	h_stride_ = *((int *)temp_buf);
	ptr += sizeof(int);

	memcpy(temp_buf, ptr, sizeof(int));
	group_ = *((int *)temp_buf);
	ptr += sizeof(int);

	// 不使用（应该是在）
	memcpy(temp_buf, ptr, sizeof(int));
	bias_term_ = *((int *)temp_buf);
	ptr += sizeof(int);

	memcpy(temp_buf, ptr, sizeof(float));
	output_multi_rate_ = *((float *)temp_buf);
	ptr += sizeof(float);

	memcpy(temp_buf, ptr, sizeof(int));
	bool_16Bit_32Bit_Add_ = *((int *)temp_buf);
	ptr += sizeof(int);

	half_kernel_size_ = kernel_size_ / 2;

	model_size_ = output_channel*kernel_size_*kernel_size_*input_channel / group_;
	bias_size_ = output_channel;

	input_channel_ = input_channel;
	output_channel_ = output_channel;

	// order: Output Channel x Input Channel x K x K

	fixed_weight_ = (FixedConvType *)ptr;
	ptr += sizeof(FixedConvType)*model_size_;

	// // 为什么要跳过一段内存 @zhangjian
	// if (model_size_%2)
	// 	ptr += sizeof(FixedConvType);

	fixed_bias_value_ = (FixedBiasType *)ptr;
	ptr += sizeof(FixedBiasType)*output_channel;

	EAGLEEYE_SAFEFREE(weight_group_rate_);
	weight_group_rate_ = (float *)malloc(sizeof(float)*output_channel);
	memcpy(weight_group_rate_, ptr, sizeof(float)*output_channel);
	ptr += sizeof(float)*output_channel;
//     if(cpu_gpu_mode == RECOG_CNN_OPENCL_MODE)
//     {
// #ifdef CNN_USING_OPENCL_GPU_PROCESSING
// 		int new_output_channel = (output_channel + 3) / 4 * 4;
// 		int new_input_channel = (input_channel + 3) / 4 * 4;

// 		FixedConvType *new_weight = (FixedConvType *)malloc(sizeof(FixedConvType)*new_output_channel*new_input_channel*kernel_size*kernel_size);
// 		FixedType *new_bias_value = (FixedType *)malloc(sizeof(FixedType)*new_output_channel);
// 		int i = 0, j = 0, k = 0, m = 0, l1 = 0, l2 = 0, n = 0;
// 		n = 0;
// 		memset(new_bias_value, 0, sizeof(FixedType)*new_output_channel);
// 		memcpy(new_bias_value, fixed_bias_value, sizeof(FixedType)*output_channel);
// 		memset(new_weight, 0, sizeof(FixedConvType)*new_input_channel*new_output_channel*kernel_size*kernel_size);
// 		for (k = 0; k < new_output_channel; k +=4)
// 		{
// 			for (i = 0; i < kernel_size*kernel_size; ++i)
// 				for (j = 0; j < new_input_channel; j+=4)
// 				{
// 					int inc_index = j;
// 					for (int l1 = 0; l1 < 4; ++l1)
// 					{
// 						int outc_index = k;

// 						for (int l2 = 0; l2 < 4; ++l2)
// 						{
// 							if (outc_index >= output_channel || inc_index >= input_channel)
// 								new_weight[n] = 0;
// 							else
// 							{
// 								new_weight[n] = fixed_weight[outc_index*input_channel*kernel_size*kernel_size + inc_index*kernel_size*kernel_size + i];
// 							}
// 							outc_index++;
// 							n++;
// 						}
// 						inc_index++;
// 					}
// 				}
// 		}

// 		CNN_OPENCL_RESOURCE &ocl_rsc = CNN_OPENCL_RESOURCE::getInstance();
// 		new_weight_cl = clCreateBuffer(ocl_rsc.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(FixedConvType)* new_output_channel*new_input_channel*kernel_size*kernel_size, new_weight, NULL);
// 		if(new_weight_cl == NULL) 		return false;

// 		new_bias_value_cl = clCreateBuffer(ocl_rsc.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(FixedType)* new_output_channel, new_bias_value, NULL);
// 		if(new_bias_value_cl == NULL) 		return false;
// 		free(new_weight);
// 		free(new_bias_value);

// #endif
//     }
	size = ptr - buf;    
	if (size != in_size)
		return false;
	else
		return true;
}

int FixedConvOp::setShape(std::vector<int64_t> shape){
    this->m_input_shape.resize(input_data_num_);
    this->m_output_shape.resize(output_data_num_);

    this->m_input_shape[0] = shape;
    std::vector<int64_t> out_shape(4);
    out_shape[3] = (m_input_shape[0][3] + 2 * pad_size_ - kernel_size_) / w_stride_ + 1;
    out_shape[2] = (m_input_shape[0][2] + 2 * pad_size_ - kernel_size_) / h_stride_ + 1;
    out_shape[1] = output_channel_;
    out_shape[0] = 1;

	this->m_output_shape[0] = out_shape;
    return 0;
}
}
} // namespace eagleeye
