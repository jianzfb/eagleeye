#include "eagleeye/engine/nano/op/preprocess_op.h"
#include <stdint.h>
#include <arm_neon.h>

namespace eagleeye{
namespace dataflow{
PreprocessOp::PreprocessOp(std::vector<float> mean_v, std::vector<float> scale_v, bool reverse_channel)
    :m_mean_v(mean_v),
        m_scale_v(scale_v),
        m_reverse_channel(reverse_channel){

}

PreprocessOp::PreprocessOp(const PreprocessOp& op)
    :m_mean_v(op.m_mean_v),
        m_scale_v(op.m_scale_v),
        m_reverse_channel(op.m_reverse_channel){

}

PreprocessOp::~PreprocessOp(){

}

int PreprocessOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}


void PreprocessOp::bgrToTensorCHW(const uint8_t* src,
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
#pragma omp parallel for
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

void PreprocessOp::bgrToRgbTensorCHW(const uint8_t* src,
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
#pragma omp parallel for
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

int PreprocessOp::runOnCpu(std::vector<Tensor> input){
    Tensor x = input[0];
    Dim x_dim = x.dims();
        
    if(x.type() != EAGLEEYE_UCHAR){
        EAGLEEYE_LOGE("PreprocessOp only support EAGLEEYE_UCHAR.");
        return -1;
    }

    if(x.format() != DataFormat::NHWC){
        EAGLEEYE_LOGE("PreprocessOp only support NHWC.");
        return -1;
    }

    if(x_dim[3] != 3){
        EAGLEEYE_LOGE("PreprocessOp only support channel=3.");
        return -1;
    }

    if(this->m_outputs[0].numel() != input[0].numel()){
        // 分配空间
        std::vector<int64_t> shape(4);
        shape[0] = x_dim[0];
        shape[1] = x_dim[3];
        shape[2] = x_dim[1];
        shape[3] = x_dim[2];
        this->m_outputs[0] =          
                Tensor(shape,
                        EAGLEEYE_FLOAT,
                        DataFormat::NCHW,
                        CPU_BUFFER); 
    }

    uint8_t* src_ptr = (uint8_t*)(x.cpu());
    float* output_ptr = (float*)(this->m_outputs[0].cpu());
    if(this->m_reverse_channel){
        this->bgrToRgbTensorCHW(
            src_ptr,
            output_ptr,
            x_dim[2],
            x_dim[1],
            (float*)(m_mean_v.data()),
            (float*)(m_scale_v.data())
        );
    }
    else{
        this->bgrToTensorCHW(
            src_ptr,
            output_ptr,
            x_dim[2],
            x_dim[1],
            (float*)(m_mean_v.data()),
            (float*)(m_scale_v.data())
        );
    }
    return 0;
}

int PreprocessOp::runOnGpu(std::vector<Tensor> input){
    return 0;
}
} // namespace dataflow
    
} // namespace eagleeye
