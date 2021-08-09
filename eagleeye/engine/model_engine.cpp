#include "eagleeye/engine/model_engine.h"
#include <iostream>
#include <dlfcn.h>
#include <stdint.h>
#include <arm_neon.h>

namespace eagleeye{
static void where_am_i() {}		
ModelEngine::ModelEngine(std::string model_name, 
				std::string device, 
				std::vector<std::string> input_names,
		     	std::vector<std::vector<int64_t>> input_shapes,
		     	std::vector<std::string> output_names,
		     	std::vector<std::vector<int64_t>> output_shapes,
		     	int omp_num_threads,
		     	RunPower model_power,
		     	std::string writable_path){
	this->m_omp_num_threads = omp_num_threads;
  this->m_model_power = model_power;
	this->m_writable_path = writable_path;
	this->m_device = device;
	this->m_model_name = model_name;

	this->m_input_names = input_names;
	this->m_output_names = output_names;

  for(int index=0; index < input_names.size(); ++index){
    m_input_map_index[input_names[index]] = index;
  }
  for(int index=0; index < output_names.size(); ++index){
    m_output_map_index[output_names[index]] = index;
  }

	this->m_is_dynamic_input_shape = false;
	this->m_is_dynamic_output_shape = false;
	for(int i=0; i<input_shapes.size(); ++i){
		for(int j=0; j<input_shapes[i].size(); ++j){
			if(input_shapes[i][j] < 0){
				this->m_is_dynamic_input_shape = true;
			}
		}
	}
	if(!this->m_is_dynamic_input_shape){
		this->m_input_shapes = input_shapes;
	}
	// if(input_shapes.size() == 0){
	// 	this->m_is_dynamic_input_shape = true;
	// }

	for(int i=0; i<output_shapes.size(); ++i){
		for(int j=0; j<output_shapes[i].size(); ++j){
			if(output_shapes[i][j] < 0){
				this->m_is_dynamic_output_shape = true;
			}
		}
	}
	if(!this->m_is_dynamic_output_shape){
		this->m_output_shapes = output_shapes;
	}
	// if(output_shapes.size() == 0){
	// 	this->m_is_dynamic_output_shape = true;
	// }

	Dl_info info;
	dladdr((void*)&where_am_i, &info);
	this->m_root = info.dli_fname;
	this->m_root = this->m_root.substr(0, this->m_root.rfind('/'));
}

ModelEngine::~ModelEngine(){
}

void ModelEngine::setModelFolder(std::string model_folder){
  this->m_model_folder = model_folder;
}

std::string ModelEngine::getModelFolder(){
  return this->m_model_folder;
}

bool ModelEngine::isDynamicInputShape(){
	return this->m_is_dynamic_input_shape;
}

bool ModelEngine::isDynamicOutputShape(){
	return this->m_is_dynamic_output_shape;
}

void ModelEngine::setInputShapes(std::vector<std::vector<int64_t>> input_shapes){
	this->m_input_shapes = input_shapes;
}

void ModelEngine::setInputNames(std::vector<std::string> input_names){
	this->m_input_names = input_names;
}

std::vector<std::string> ModelEngine::getInputNames(){
	return this->m_input_names;
}

void ModelEngine::setOutputShapes(std::vector<std::vector<int64_t>> output_shapes){
	this->m_output_shapes = output_shapes;
}

void ModelEngine::setOutputNames(std::vector<std::string> output_names){
	this->m_output_names = output_names;
}

std::vector<std::string> ModelEngine::getOutputNames(){
	return this->m_output_names;
}

void ModelEngine::setOutputNameMap(std::map<std::string, std::string> name_map){
	this->m_output_name_map = name_map; 
	this->m_output_name_map2 = name_map; 

	std::map<std::string,std::string>::iterator iter;
	for(iter=name_map.begin(); iter != name_map.end(); ++iter){
		this->m_inv_output_name_map[iter->second] = iter->first;
		this->m_inv_output_name_map2[iter->second] = iter->first;
	}
}

void ModelEngine::setOutputNameMap2(std::map<std::string, std::string> name_map){
	this->m_output_name_map2 = name_map; 

	std::map<std::string,std::string>::iterator iter;
	for(iter=name_map.begin(); iter != name_map.end(); ++iter){
		this->m_inv_output_name_map2[iter->second] = iter->first;
	}
}

void ModelEngine::setInputNameMap(std::map<std::string, std::string> name_map){
	this->m_input_name_map = name_map;
		std::map<std::string,std::string>::iterator iter;
	for(iter=name_map.begin(); iter != name_map.end(); ++iter){
		this->m_inv_input_name_map[iter->second] = iter->first;
	}
}

std::string ModelEngine::getModelRoot(){
	return this->m_root;
}

void ModelEngine::setWritablePath(std::string writable_path){
	this->m_writable_path = writable_path;
}

std::string ModelEngine::getWritablePath(){
	return this->m_writable_path;
}


void ModelEngine::bgrToTensorCHW(const uint8_t* src,
                       float* output,
                       int width,
                       int height,
                       float* means,
                       float* scales) {
  int size = width * height;
  float b_means = means[0];
  float g_means = means[1];
  float r_means = means[2];
  float b_scales = scales[0];
  float g_scales = scales[1];
  float r_scales = scales[2];

  float* ptr_b = output;
  float* ptr_g = ptr_b + size;
  float* ptr_r = ptr_g + size;

  int dim8 = width >> 3;
  int remain = width % 8;

  float32x4_t vbmean = vdupq_n_f32(b_means);
  float32x4_t vgmean = vdupq_n_f32(g_means);
  float32x4_t vrmean = vdupq_n_f32(r_means);
  float32x4_t vbscale = vdupq_n_f32(b_scales);
  float32x4_t vgscale = vdupq_n_f32(g_scales);
  float32x4_t vrscale = vdupq_n_f32(r_scales);
// #pragma omp parallel for
  for (int i = 0; i < height; i += 1) {
    const uint8_t* din_ptr = src + i * 3 * width;
    float* ptr_b_h = ptr_b + i * width;
    float* ptr_g_h = ptr_g + i * width;
    float* ptr_r_h = ptr_r + i * width;
    int cnt = dim8;
    if (cnt > 0) {
#ifdef __aarch64__
      asm volatile(
          "prfm   pldl1keep, [%[inptr0]]                \n"
          "prfm   pldl1keep, [%[inptr0], #64]   \n"
          "prfm   pldl1keep, [%[inptr0], #128]   \n"
          "prfm   pldl1keep, [%[inptr0], #192]   \n"
          "1:     \n"
          "ld3 {v0.8b, v1.8b, v2.8b}, [%[inptr0]], #24 \n"  // d8 = y0y3y6y9..
                                                            // d9 = y1y4y7..."
          // 8->16
          "ushll v3.8h, v0.8b, #0  \n"
          "ushll v4.8h, v1.8b, #0  \n"
          "ushll v5.8h, v2.8b, #0  \n"
          // 16->32
          "ushll v6.4s, v3.4h, #0   \n"
          "ushll2 v7.4s, v3.8h, #0   \n"
          "ushll v8.4s, v4.4h, #0   \n"
          "ushll2 v9.4s, v4.8h, #0   \n"
          "ushll v10.4s, v5.4h, #0  \n"
          "ushll2 v11.4s, v5.8h, #0   \n"
          // int32->fp32
          "ucvtf v12.4s, v6.4s \n"
          "ucvtf v13.4s, v7.4s \n"
          "ucvtf v14.4s, v8.4s \n"
          "ucvtf v15.4s, v9.4s \n"
          "ucvtf v16.4s, v10.4s \n"
          "ucvtf v17.4s, v11.4s \n"
          // sub -mean
          "fsub v12.4s, v12.4s, %[vbmean].4s \n"
          "fsub v13.4s, v13.4s, %[vbmean].4s \n"
          "fsub v14.4s, v14.4s, %[vgmean].4s \n"
          "fsub v15.4s, v15.4s, %[vgmean].4s \n"
          "fsub v16.4s, v16.4s, %[vrmean].4s \n"
          "fsub v17.4s, v17.4s, %[vrmean].4s \n"
          // mul * scale
          "fmul v6.4s, v12.4s, %[vbscale].4s \n"
          "fmul v7.4s, v13.4s, %[vbscale].4s \n"
          "fmul v8.4s, v14.4s, %[vgscale].4s \n"
          "fmul v9.4s, v15.4s, %[vgscale].4s \n"
          "fmul v10.4s, v16.4s, %[vrscale].4s \n"
          "fmul v11.4s, v17.4s, %[vrscale].4s \n"
          // store
          "st1 {v6.4s}, [%[outr0]], #16 \n"
          "st1 {v8.4s}, [%[outr1]], #16 \n"
          "st1 {v10.4s}, [%[outr2]], #16 \n"
          "subs %w[cnt], %w[cnt], #1 \n"
          "st1 {v7.4s}, [%[outr0]], #16 \n"
          "st1 {v9.4s}, [%[outr1]], #16 \n"
          "st1 {v11.4s}, [%[outr2]], #16 \n"
          "bne 1b \n"
          : [inptr0] "+r"(din_ptr),
            [outr0] "+r"(ptr_b_h),
            [outr1] "+r"(ptr_g_h),
            [outr2] "+r"(ptr_r_h),
            [cnt] "+r"(cnt)
          : [vbmean] "w"(vbmean),
            [vgmean] "w"(vgmean),
            [vrmean] "w"(vrmean),
            [vbscale] "w"(vbscale),
            [vgscale] "w"(vgscale),
            [vrscale] "w"(vrscale)
          : "cc",
            "memory",
            "v0",
            "v1",
            "v2",
            "v3",
            "v4",
            "v5",
            "v6",
            "v7",
            "v8",
            "v9",
            "v10",
            "v11",
            "v12",
            "v13",
            "v14",
            "v15",
            "v16",
            "v17",
            "v18",
            "v19",
            "v20");
#else
      asm volatile(
          "pld [%[inptr0]]                         @ preload a, 64byte\n"
          "pld [%[inptr0], #64]                         @ preload a, 64byte\n"
          "pld [%[inptr0], #128]                         @ preload a, 64byte\n"
          "pld [%[inptr0], #192]                         @ preload a, 64byte\n"
          "1: \n"
          "vld3.8 {d12, d13, d14}, [%[inptr0]]! \n"
          // 8->16
          "vmovl.u8 q8, d12 \n"
          "vmovl.u8 q9, d13 \n"
          "vmovl.u8 q10, d14 \n"
          // 16->32
          "vmovl.u16 q11, d16 \n"
          "vmovl.u16 q12, d17 \n"
          "vmovl.u16 q13, d18 \n"
          "vmovl.u16 q14, d19 \n"
          "vmovl.u16 q15, d20 \n"
          "vmovl.u16 q6, d21 \n"
          // int32->fp32
          "vcvt.f32.u32 q7, q11 \n"
          "vcvt.f32.u32 q8, q12 \n"
          "vcvt.f32.u32 q9, q13 \n"
          "vcvt.f32.u32 q10, q14 \n"
          "vcvt.f32.u32 q11, q15 \n"
          "vcvt.f32.u32 q12, q6 \n"
          // sub -mean
          "vsub.f32 q7, q7, %q[vbmean] \n"
          "vsub.f32 q8, q8, %q[vbmean] \n"
          "vsub.f32 q9, q9, %q[vgmean] \n"
          "vsub.f32 q10, q10, %q[vgmean] \n"
          "vsub.f32 q11, q11, %q[vrmean] \n"
          "vsub.f32 q12, q12, %q[vrmean] \n"
          // mul *scale
          "vmul.f32 q13, q7, %q[vbscale] \n"
          "vmul.f32 q14, q8, %q[vbscale] \n"
          "vmul.f32 q15, q9, %q[vgscale] \n"
          "vmul.f32 q6, q10, %q[vgscale] \n"
          "vmul.f32 q7, q11, %q[vrscale] \n"
          "vmul.f32 q8, q12, %q[vrscale] \n"
          // store
          "vst1.32  {d26 - d27}, [%[outr0]]! \n"
          "vst1.32  {d30 - d31}, [%[outr1]]! \n"
          "vst1.32  {d14 - d15}, [%[outr2]]! \n"
          "subs %[cnt], #1 \n"
          "vst1.32  {d28 - d29}, [%[outr0]]! \n"
          "vst1.32  {d12 - d13}, [%[outr1]]! \n"
          "vst1.32  {d16 - d17}, [%[outr2]]! \n"
          "bne 1b"
          : [inptr0] "+r"(din_ptr),
            [outr0] "+r"(ptr_b_h),
            [outr1] "+r"(ptr_g_h),
            [outr2] "+r"(ptr_r_h),
            [cnt] "+r"(cnt)
          : [vbmean] "w"(vbmean),
            [vgmean] "w"(vgmean),
            [vrmean] "w"(vrmean),
            [vbscale] "w"(vbscale),
            [vgscale] "w"(vgscale),
            [vrscale] "w"(vrscale)
          : "cc",
            "memory",
            "q6",
            "q7",
            "q8",
            "q9",
            "q10",
            "q11",
            "q12",
            "q13",
            "q14",
            "q15");
#endif
    }
    for (int j = 0; j < remain; j++) {
      *ptr_b_h++ = (*din_ptr - b_means) * b_scales;
      din_ptr++;
      *ptr_g_h++ = (*din_ptr - g_means) * g_scales;
      din_ptr++;
      *ptr_r_h++ = (*din_ptr - r_means) * r_scales;
      din_ptr++;
    }
  }
}

void ModelEngine::bgrToRgbTensorCHW(const uint8_t* src,
                       float* output,
                       int width,
                       int height,
                       float* means,
                       float* scales) {
  int size = width * height;
  float b_means = means[0];
  float g_means = means[1];
  float r_means = means[2];
  float b_scales = scales[0];
  float g_scales = scales[1];
  float r_scales = scales[2];

  float* ptr_b = output;
  float* ptr_g = ptr_b + size;
  float* ptr_r = ptr_g + size;

  int dim8 = width >> 3;
  int remain = width % 8;

  float32x4_t vbmean = vdupq_n_f32(b_means);
  float32x4_t vgmean = vdupq_n_f32(g_means);
  float32x4_t vrmean = vdupq_n_f32(r_means);
  float32x4_t vbscale = vdupq_n_f32(b_scales);
  float32x4_t vgscale = vdupq_n_f32(g_scales);
  float32x4_t vrscale = vdupq_n_f32(r_scales);
// #pragma omp parallel for
  for (int i = 0; i < height; i += 1) {
    const uint8_t* din_ptr = src + i * 3 * width;
    float* ptr_b_h = ptr_b + i * width;
    float* ptr_g_h = ptr_g + i * width;
    float* ptr_r_h = ptr_r + i * width;
    int cnt = dim8;
    if (cnt > 0) {
#ifdef __aarch64__
      asm volatile(
          "prfm   pldl1keep, [%[inptr0]]                \n"
          "prfm   pldl1keep, [%[inptr0], #64]   \n"
          "prfm   pldl1keep, [%[inptr0], #128]   \n"
          "prfm   pldl1keep, [%[inptr0], #192]   \n"
          "1:     \n"
          "ld3 {v0.8b, v1.8b, v2.8b}, [%[inptr0]], #24 \n"  // d8 = y0y3y6y9..
                                                            // d9 = y1y4y7..."
          // 8->16
          "ushll v3.8h, v2.8b, #0  \n"
          "ushll v4.8h, v1.8b, #0  \n"
          "ushll v5.8h, v0.8b, #0  \n"
          // 16->32
          "ushll v6.4s, v3.4h, #0   \n"
          "ushll2 v7.4s, v3.8h, #0   \n"
          "ushll v8.4s, v4.4h, #0   \n"
          "ushll2 v9.4s, v4.8h, #0   \n"
          "ushll v10.4s, v5.4h, #0  \n"
          "ushll2 v11.4s, v5.8h, #0   \n"
          // int32->fp32
          "ucvtf v12.4s, v6.4s \n"
          "ucvtf v13.4s, v7.4s \n"
          "ucvtf v14.4s, v8.4s \n"
          "ucvtf v15.4s, v9.4s \n"
          "ucvtf v16.4s, v10.4s \n"
          "ucvtf v17.4s, v11.4s \n"
          // sub -mean
          "fsub v12.4s, v12.4s, %[vbmean].4s \n"
          "fsub v13.4s, v13.4s, %[vbmean].4s \n"
          "fsub v14.4s, v14.4s, %[vgmean].4s \n"
          "fsub v15.4s, v15.4s, %[vgmean].4s \n"
          "fsub v16.4s, v16.4s, %[vrmean].4s \n"
          "fsub v17.4s, v17.4s, %[vrmean].4s \n"
          // mul * scale
          "fmul v6.4s, v12.4s, %[vbscale].4s \n"
          "fmul v7.4s, v13.4s, %[vbscale].4s \n"
          "fmul v8.4s, v14.4s, %[vgscale].4s \n"
          "fmul v9.4s, v15.4s, %[vgscale].4s \n"
          "fmul v10.4s, v16.4s, %[vrscale].4s \n"
          "fmul v11.4s, v17.4s, %[vrscale].4s \n"
          // store
          "st1 {v6.4s}, [%[outr0]], #16 \n"
          "st1 {v8.4s}, [%[outr1]], #16 \n"
          "st1 {v10.4s}, [%[outr2]], #16 \n"
          "subs %w[cnt], %w[cnt], #1 \n"
          "st1 {v7.4s}, [%[outr0]], #16 \n"
          "st1 {v9.4s}, [%[outr1]], #16 \n"
          "st1 {v11.4s}, [%[outr2]], #16 \n"
          "bne 1b \n"
          : [inptr0] "+r"(din_ptr),
            [outr0] "+r"(ptr_b_h),
            [outr1] "+r"(ptr_g_h),
            [outr2] "+r"(ptr_r_h),
            [cnt] "+r"(cnt)
          : [vbmean] "w"(vbmean),
            [vgmean] "w"(vgmean),
            [vrmean] "w"(vrmean),
            [vbscale] "w"(vbscale),
            [vgscale] "w"(vgscale),
            [vrscale] "w"(vrscale)
          : "cc",
            "memory",
            "v0",
            "v1",
            "v2",
            "v3",
            "v4",
            "v5",
            "v6",
            "v7",
            "v8",
            "v9",
            "v10",
            "v11",
            "v12",
            "v13",
            "v14",
            "v15",
            "v16",
            "v17",
            "v18",
            "v19",
            "v20");
#else
      asm volatile(
          "pld [%[inptr0]]                         @ preload a, 64byte\n"
          "pld [%[inptr0], #64]                         @ preload a, 64byte\n"
          "pld [%[inptr0], #128]                         @ preload a, 64byte\n"
          "pld [%[inptr0], #192]                         @ preload a, 64byte\n"
          "1: \n"
          "vld3.8 {d12, d13, d14}, [%[inptr0]]! \n"
          // 8->16
          "vmovl.u8 q8, d14 \n"
          "vmovl.u8 q9, d13 \n"
          "vmovl.u8 q10, d12 \n"
          // 16->32
          "vmovl.u16 q11, d16 \n"
          "vmovl.u16 q12, d17 \n"
          "vmovl.u16 q13, d18 \n"
          "vmovl.u16 q14, d19 \n"
          "vmovl.u16 q15, d20 \n"
          "vmovl.u16 q6, d21 \n"
          // int32->fp32
          "vcvt.f32.u32 q7, q11 \n"
          "vcvt.f32.u32 q8, q12 \n"
          "vcvt.f32.u32 q9, q13 \n"
          "vcvt.f32.u32 q10, q14 \n"
          "vcvt.f32.u32 q11, q15 \n"
          "vcvt.f32.u32 q12, q6 \n"
          // sub -mean
          "vsub.f32 q7, q7, %q[vbmean] \n"
          "vsub.f32 q8, q8, %q[vbmean] \n"
          "vsub.f32 q9, q9, %q[vgmean] \n"
          "vsub.f32 q10, q10, %q[vgmean] \n"
          "vsub.f32 q11, q11, %q[vrmean] \n"
          "vsub.f32 q12, q12, %q[vrmean] \n"
          // mul *scale
          "vmul.f32 q13, q7, %q[vbscale] \n"
          "vmul.f32 q14, q8, %q[vbscale] \n"
          "vmul.f32 q15, q9, %q[vgscale] \n"
          "vmul.f32 q6, q10, %q[vgscale] \n"
          "vmul.f32 q7, q11, %q[vrscale] \n"
          "vmul.f32 q8, q12, %q[vrscale] \n"
          // store
          "vst1.32  {d26 - d27}, [%[outr0]]! \n"
          "vst1.32  {d30 - d31}, [%[outr1]]! \n"
          "vst1.32  {d14 - d15}, [%[outr2]]! \n"
          "subs %[cnt], #1 \n"
          "vst1.32  {d28 - d29}, [%[outr0]]! \n"
          "vst1.32  {d12 - d13}, [%[outr1]]! \n"
          "vst1.32  {d16 - d17}, [%[outr2]]! \n"
          "bne 1b"
          : [inptr0] "+r"(din_ptr),
            [outr0] "+r"(ptr_b_h),
            [outr1] "+r"(ptr_g_h),
            [outr2] "+r"(ptr_r_h),
            [cnt] "+r"(cnt)
          : [vbmean] "w"(vbmean),
            [vgmean] "w"(vgmean),
            [vrmean] "w"(vrmean),
            [vbscale] "w"(vbscale),
            [vgscale] "w"(vgscale),
            [vrscale] "w"(vrscale)
          : "cc",
            "memory",
            "q6",
            "q7",
            "q8",
            "q9",
            "q10",
            "q11",
            "q12",
            "q13",
            "q14",
            "q15");
#endif
    }
    for (int j = 0; j < remain; j++) {
      *ptr_r_h++ = (*din_ptr - b_means) * b_scales;
      din_ptr++;
      *ptr_g_h++ = (*din_ptr - g_means) * g_scales;
      din_ptr++;
      *ptr_b_h++ = (*din_ptr - r_means) * r_scales;
      din_ptr++;
    }
  }
}
}