#include "eagleeye/engine/nano/op/transpose_op.h"
#include <string>
#include <arm_neon.h>

namespace eagleeye{
namespace dataflow{
TransposeOp::TransposeOp(std::vector<int64_t> axis){
    OP_SUPPORT(CPU);
    m_need_trans = false;
    m_trans_mat = false;
    m_trans_num = 0;
    m_trans_w = 0;
    m_trans_h = 0;
    this->m_axis = axis;
    this->m_new_shape.ConstructFrom(axis);
}

TransposeOp::TransposeOp(const TransposeOp& op){
    m_need_trans = false;
    m_trans_mat = false;
    m_trans_num = 0;
    m_trans_w = 0;
    m_trans_h = 0;
    this->m_axis = op.m_axis;
    this->m_new_shape.ConstructFrom(op.m_axis);
}

TransposeOp::~TransposeOp(){

}

int TransposeOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

template <typename Dtype>
void transpose_mat(const Dtype* din,
                   Dtype* dout,
                   const int num,
                   const int width,
                   const int height);
#define INIT_PTR_4(dtype, ptr_out, size_h)            \
  dtype* data_out_ptr = ptr_out + w * size_h + tmp_h; \
  const dtype* din0 = ptr_din_row;                    \
  const dtype* din1 = din0 + width;                   \
  const dtype* din2 = din1 + width;                   \
  const dtype* din3 = din2 + width;                   \
  dtype* dout0 = data_out_ptr;                        \
  dtype* dout1 = dout0 + height;                      \
  dtype* dout2 = dout1 + height;                      \
  dtype* dout3 = dout2 + height;

#define INIT_PTR_A4(dtype)          \
  const dtype* din4 = din3 + width; \
  const dtype* din5 = din4 + width; \
  const dtype* din6 = din5 + width; \
  const dtype* din7 = din6 + width;

#define INIT_PTR_B4(dtype)       \
  dtype* dout4 = dout3 + height; \
  dtype* dout5 = dout4 + height; \
  dtype* dout6 = dout5 + height; \
  dtype* dout7 = dout6 + height;


template <typename Dtype>
void TransposeCompute_(const std::vector<int64_t>& axis,
                       Tensor input,
                       Tensor output) {
  Dtype* input_ptr = (Dtype*)input.cpu();
  Dtype* output_ptr = (Dtype*)output.cpu();

  // input and output's shape dimension must >= 2 && <= 6.
  const Dim& in_dim = input.dims();
  const Dim& out_dim = output.dims();

  // precompute inverted output dim and strides
  size_t rout_dim[6], strides[6];
  int permute = axis.size();  // permute must >=2 && <= 6.
  for (int i = 0; i < permute; ++i) {
    int k = permute - 1 - i;
    strides[k] = 1;
    for (int j = axis[i] + 1; j < permute; ++j) {
      strides[k] *= in_dim[j];
    }
    rout_dim[k] = out_dim[i];
  }

  // unroll the first 2 dimensions
  int reamin_dim = 1;
  for (int i = 2; i < out_dim.size(); ++i) {
    reamin_dim *= out_dim[i];
  }

#pragma omp parallel for collapse(2)
  for (int batch = 0; batch < out_dim[0]; ++batch) {
    for (int j = 0; j < out_dim[1]; ++j) {
      size_t offset = batch * strides[permute - 1] + j * strides[permute - 2];
      Dtype* out_ptr = output_ptr + (batch * out_dim[1] + j) * reamin_dim;
      int indics[4] = {0, 0, 0, 0};
      for (int k = 0; k < reamin_dim; ++k) {
        out_ptr[k] = input_ptr[offset];
        indics[0] += 1;
        offset += strides[0];
        for (int p = 0; p < permute - 3; ++p) {
          if (indics[p] == rout_dim[p]) {
            indics[p + 1] += 1;
            indics[p] = 0;
            offset += strides[p + 1];
            offset -= rout_dim[p] * strides[p];
          } else {
            break;
          }
        }
      }
    }
  }
}

void transpose_mat(const float* din,
                   float* dout,
                   const int num,
                   const int width,
                   const int height) {
  int nw = width >> 2;
  int nh = height >> 2;
  int size_in = width * height;
  int size_w = width << 2;
  int size_h = height << 2;

  for (int i = 0; i < num; ++i) {
    float* ptr_out = dout + i * size_in;
    const float* ptr_in = din + i * size_in;
#pragma omp parallel for
    for (int h = 0; h < nh; h++) {
      const float* ptr_din_row = ptr_in + h * size_w;
      int tmp_h = h * 4;
      for (int w = 0; w < nw; w++) {
        INIT_PTR_4(float, ptr_out, size_h)
#ifdef __aarch64__
        float32x4_t vr0 = vld1q_f32(din0);
        float32x4_t vr1 = vld1q_f32(din1);
        float32x4_t vr2 = vld1q_f32(din2);
        float32x4_t vr3 = vld1q_f32(din3);
        float32x4_t re0 = vtrn1q_f32(vr0, vr1);
        float32x4_t re1 = vtrn2q_f32(vr0, vr1);
        float32x4_t re2 = vtrn1q_f32(vr2, vr3);
        float32x4_t re3 = vtrn2q_f32(vr2, vr3);
        vst1_f32(dout0, vget_low_f32(re0));
        dout0 += 2;
        vst1_f32(dout0, vget_low_f32(re2));
        vst1_f32(dout1, vget_low_f32(re1));
        dout1 += 2;
        vst1_f32(dout1, vget_low_f32(re3));
        vst1_f32(dout2, vget_high_f32(re0));
        dout2 += 2;
        vst1_f32(dout2, vget_high_f32(re2));
        vst1_f32(dout3, vget_high_f32(re1));
        dout3 += 2;
        vst1_f32(dout3, vget_high_f32(re3));
#else
        asm("vld1.32 {d0, d1}, [%[in0]]    \n"
            "vld1.32 {d2, d3}, [%[in1]]    \n"
            "vld1.32 {d4, d5}, [%[in2]]    \n"
            "vld1.32 {d6, d7}, [%[in3]]    \n"
            "vtrn.32 q0, q1                \n"
            "vtrn.32 q2, q3                \n"
            "vswp d1, d4                   \n"
            "vswp d3, d6                   \n"
            "vst1.32 {d0, d1}, [%[out0]]   \n"
            "vst1.32 {d2, d3}, [%[out1]]   \n"
            "vst1.32 {d4, d5}, [%[out2]]   \n"
            "vst1.32 {d6, d7}, [%[out3]]   \n"
            :
            : [out0] "r"(dout0),
              [out1] "r"(dout1),
              [out2] "r"(dout2),
              [out3] "r"(dout3),
              [in0] "r"(din0),
              [in1] "r"(din1),
              [in2] "r"(din2),
              [in3] "r"(din3)
            : "q0", "q1", "q2", "q3");
#endif
        ptr_din_row += 4;
      }
    }
    // remian
    for (int h = 0; h < height; h++) {
      for (int w = nw * 4; w < width; w++) {
        const float* data_in_ptr = ptr_in + h * width + w;
        float* data_out_ptr = ptr_out + w * height + h;
        *data_out_ptr = *data_in_ptr;
      }
    }
    for (int w = 0; w < width; w++) {
      for (int h = nh * 4; h < height; h++) {
        const float* data_in_ptr = ptr_in + h * width + w;
        float* data_out_ptr = ptr_out + w * height + h;
        *data_out_ptr = *data_in_ptr;
      }
    }
  }
}


int TransposeOp::runOnCpu(std::vector<Tensor> input){
    // 1.step 构建变换参数
    this->reInitWhenNeeded(input[0]);

    // 2.step 首次运行分配空间
    if(input[0].type() != EAGLEEYE_FLOAT){
      EAGLEEYE_LOGE("x type only support float.");
      return -1;
    }

    if(this->m_outputs[0].numel() != input[0].numel()){
        // 分配空间
        std::vector<int64_t> shape(m_axis.size());

        for(int i=0; i<m_axis.size(); ++i){ 
          shape[i] = input[0].dims()[m_axis[i]];
        }
        this->m_outputs[0] =          
                Tensor(shape,
                        input[0].type(),
                        DataFormat::AUTO,
                        CPU_BUFFER); 
    }

    // 3.step here, computing
    if(!m_need_trans){
        float* out_ptr = (float*)this->m_outputs[0].cpu();
        float* in_ptr = (float*)input[0].cpu();

        memcpy(out_ptr, in_ptr, sizeof(float)*input[0].numel());
        return 0;
    }

    if(m_trans_mat){
        float* din = (float*)input[0].cpu();
        float* dout = (float*)this->m_outputs[0].cpu();
        transpose_mat(din, dout, m_trans_num, m_trans_w, m_trans_h);
        return 0;
    }

    TransposeCompute_<float>(m_new_shape.data(), input[0], this->m_outputs[0]);
    return 0;
}

int TransposeOp::runOnGpu(std::vector<Tensor> input){
    EAGLEEYE_LOGE("Dont implement (GPU)");
    return 0;
}

std::vector<int> get_stride(const Dim& dims) {
  std::vector<int> data_stride{0};

  for (int i = 0; i < dims.size(); ++i) {
    data_stride.push_back(dims.count(i, dims.size()));
  }
  return data_stride;
}

void TransposeOp::reInitWhenNeeded(Tensor input){
    Dim x_dims = m_new_shape;
    if (m_last_shape == x_dims) {
        return;
    }
    m_last_shape = x_dims;
    int _num_axes = input.dims().size();
    m_need_trans = false;
    for (int i = 0; i < _num_axes; ++i) {
        if (x_dims[i] != i) {
        m_need_trans = true;
        break;
        }
    }


    if (!m_need_trans) {
        return;
    }

    std::vector<int> axis_diff;
    int j = 0;
    for (int i = 0; i < _num_axes; ++i) {
        if (x_dims[j] != i) {
        axis_diff.push_back(j);
        } else {
        j++;
        }
    }
    for (int i = 0; i < axis_diff.size(); i++) {
    }
    if (input.dims().count(axis_diff[0], _num_axes) == 1) {
        m_need_trans = false;
        return;
    }

    if (axis_diff.size() == 1) {
        m_trans_mat = true;
        m_trans_num = input.dims().count(0, std::max(axis_diff[0], 0));
        m_trans_w = input.dims().count(axis_diff[0] + 1, _num_axes);
        m_trans_h = input.dims()[axis_diff[0]];

    } else {
        m_trans_mat = false;
    }
}
}
}