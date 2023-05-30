#include "eagleeye/engine/nano/op/roi_align_op.h"

namespace eagleeye{
namespace dataflow{
RoiAlignOp::RoiAlignOp(int64_t pooled_h, int64_t pooled_w, float spatial_scale, bool align)
    :m_pooled_h(pooled_h),
     m_pooled_w(pooled_w),
     m_spatial_scale(spatial_scale),
     m_align(align){

}
RoiAlignOp::~RoiAlignOp(){

}

int RoiAlignOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

static constexpr int kROISize = 4;
template <class T>
void PreCalcForBilinearInterpolate(const int height,
                                   const int width,
                                   const int pooled_height,
                                   const int pooled_width,
                                   const int iy_upper,
                                   const int ix_upper,
                                   T roi_ymin,
                                   T roi_xmin,
                                   T bin_size_h,
                                   T bin_size_w,
                                   int roi_bin_grid_h,
                                   int roi_bin_grid_w,
                                   int* pre_pos_data,
                                   float* pre_w_data) {
  int pre_calc_index = 0;
//   int* pre_pos_data = pre_pos->mutable_data<int>();
//   T* pre_w_data = pre_w->mutable_data<T>();
//   memset(pre_pos_data, 0, pre_pos->numel() * sizeof(int));
//   memset(pre_w_data, 0, pre_w->numel() * sizeof(T));

  for (int ph = 0; ph < pooled_height; ph++) {
    for (int pw = 0; pw < pooled_width; pw++) {
      for (int iy = 0; iy < iy_upper; iy++) {
        // calculate y of sample points
        T y = roi_ymin + ph * bin_size_h +
              static_cast<T>(iy + .5f) * bin_size_h /
                  static_cast<T>(roi_bin_grid_h);
        // calculate x of samle points
        for (int ix = 0; ix < ix_upper; ix++) {
          T x = roi_xmin + pw * bin_size_w +
                static_cast<T>(ix + .5f) * bin_size_w /
                    static_cast<T>(roi_bin_grid_w);
          // deal with elements out of map
          if (y < -1.0 || y > height || x < -1.0 || x > width) {
            for (int i = 0; i < kROISize; ++i) {
              pre_pos_data[i + pre_calc_index * kROISize] = 0;
              pre_w_data[i + pre_calc_index * kROISize] = 0;
            }
            pre_calc_index += 1;
            continue;
          }
          y = y <= 0 ? 0 : y;
          x = x <= 0 ? 0 : x;

          int y_low = static_cast<int>(y);
          int x_low = static_cast<int>(x);
          int y_high;
          int x_high;
          if (y_low >= height - 1) {
            y_high = y_low = height - 1;
            y = static_cast<T>(y_low);
          } else {
            y_high = y_low + 1;
          }
          if (x_low >= width - 1) {
            x_high = x_low = width - 1;
            x = static_cast<T>(x_low);
          } else {
            x_high = x_low + 1;
          }
          T ly = y - y_low, lx = x - x_low;
          T hy = 1. - ly, hx = 1. - lx;
          pre_pos_data[pre_calc_index * kROISize] = y_low * width + x_low;
          pre_pos_data[pre_calc_index * kROISize + 1] = y_low * width + x_high;
          pre_pos_data[pre_calc_index * kROISize + 2] = y_high * width + x_low;
          pre_pos_data[pre_calc_index * kROISize + 3] = y_high * width + x_high;
          pre_w_data[pre_calc_index * kROISize] = hy * hx;
          pre_w_data[pre_calc_index * kROISize + 1] = hy * lx;
          pre_w_data[pre_calc_index * kROISize + 2] = ly * hx;
          pre_w_data[pre_calc_index * kROISize + 3] = ly * lx;
          pre_calc_index += 1;
        }
      }
    }
  }
}

int RoiAlignOp::runOnCpu(const std::vector<Tensor>& input){
    const Tensor feat = input[0];
    const Tensor rois = input[1];
    const Tensor rois_lot = input[2]; // batch_size + 1
    if(feat.type() != EAGLEEYE_FLOAT || rois.type() != EAGLEEYE_FLOAT || rois_lot.type() != EAGLEEYE_INT){
        EAGLEEYE_LOGE("feat,rois or rois_lot type not support.");
        return -1;
    }
    if(feat.dims().size() != 4){
        EAGLEEYE_LOGE("feat must be a tensor.");
        return -1;
    }

    const int32_t* rois_lot_data = rois_lot.cpu<int32_t>();

    // feature map size
    auto in_dims = feat.dims();
    int batch_size = in_dims[0];
    int channels = in_dims[1];
    int height = in_dims[2];
    int width = in_dims[3];

    // roi size
    auto rois_dims = rois.dims();
    int rois_num = rois_dims[0];

    Dim out_dims = this->m_outputs[0].dims();
    if(out_dims.size() == 0 || out_dims.production() != rois_num * channels * m_pooled_h * m_pooled_w){
        this->m_outputs[0] =             
            Tensor(std::vector<int64_t>{rois_num, channels, m_pooled_h, m_pooled_w},
                    feat.type(),
                    DataFormat::AUTO,
                    CPU_BUFFER); 
        out_dims = this->m_outputs[0].dims();
    }
    float* output_data = this->m_outputs[0].cpu<float>();
    memset(output_data, 0, out_dims.production() * sizeof(float));

    Dim in_stride({static_cast<int64_t>(in_dims[1] * in_dims[2] * in_dims[3]),
                    static_cast<int64_t>(in_dims[2] * in_dims[3]),
                    static_cast<int64_t>(in_dims[3]),
                    1});
    Dim roi_stride({static_cast<int64_t>(rois_dims[1]), 1});
    Dim out_stride({static_cast<int64_t>(out_dims[1] * out_dims[2] * out_dims[3]),
                    static_cast<int64_t>(out_dims[2] * out_dims[3]),
                    static_cast<int64_t>(out_dims[3]),
                    1});

    int* roi_batch_id_data = (int*)malloc(sizeof(int)*rois_num);
    memset(roi_batch_id_data, 0, sizeof(int)*rois_num);
    int batch_id = 0;
    for(int roi_i=0; roi_i<rois_num; ++roi_i){
        if(roi_i >= rois_lot_data[batch_id] && roi_i < rois_lot_data[batch_id+1]){
            roi_batch_id_data[roi_i] = batch_id;
        }
        else{
            batch_id +=1;
        }
    }

    const float* input_data = feat.cpu<float>();
    const float* rois_data = rois.cpu<float>();
    float roi_offset = m_align ? 0.5f : 0.f;
    for (int n = 0; n < rois_num; ++n) {
        int roi_batch_id = roi_batch_id_data[n];
        float roi_xmin = rois_data[0] * m_spatial_scale - roi_offset;
        float roi_ymin = rois_data[1] * m_spatial_scale - roi_offset;
        float roi_xmax = rois_data[2] * m_spatial_scale - roi_offset;
        float roi_ymax = rois_data[3] * m_spatial_scale - roi_offset;
        float roi_width = roi_xmax - roi_xmin;
        float roi_height = roi_ymax - roi_ymin;
        if (!m_align) {
            roi_width = std::max(roi_width, 1.f);
            roi_height = std::max(roi_height, 1.f);
        }

        float bin_size_h = roi_height / m_pooled_h;
        float bin_size_w = roi_width / m_pooled_w;
        const float* batch_data = input_data + roi_batch_id * in_stride[0];
        int roi_bin_grid_h = ceil(roi_height / m_pooled_h);
        int roi_bin_grid_w = ceil(roi_width / m_pooled_w);

        const int count = std::max(roi_bin_grid_h * roi_bin_grid_w, 1);
        int* pre_pos_data = new int[count * out_stride[1] * kROISize];
        float* pre_w_data = new float[count * out_stride[1] * kROISize];
        PreCalcForBilinearInterpolate<float>(height,
                                            width,
                                            m_pooled_h,
                                            m_pooled_w,
                                            roi_bin_grid_h,
                                            roi_bin_grid_w,
                                            roi_ymin,
                                            roi_xmin,
                                            bin_size_h,
                                            bin_size_w,
                                            roi_bin_grid_h,
                                            roi_bin_grid_w,
                                            pre_pos_data,
                                            pre_w_data);

        for (int c = 0; c < channels; c++) {
            int pre_calc_index = 0;
            for (int ph = 0; ph < m_pooled_h; ph++) {
                for (int pw = 0; pw < m_pooled_w; pw++) {
                    const int pool_index = ph * m_pooled_w + pw;
                    float output_val = 0;
                    for (int iy = 0; iy < roi_bin_grid_h; iy++) {
                        for (int ix = 0; ix < roi_bin_grid_w; ix++) {
                            for (int i = 0; i < kROISize; i++) {
                                int pos = pre_pos_data[pre_calc_index * kROISize + i];
                                float w = pre_w_data[pre_calc_index * kROISize + i];
                                output_val += w * batch_data[pos];
                            }
                            pre_calc_index += 1;
                        }
                    }
                    output_val /= count;
                    output_data[pool_index] = output_val;
                }
            }
            batch_data += in_stride[1];
            output_data += out_stride[1];
        }
        rois_data += roi_stride[0];

        delete[] pre_pos_data;
        delete[] pre_w_data;
    }

    free(roi_batch_id_data);
    return 0;
}

int RoiAlignOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}
} // namespace dataflow
} // namespace eagleeye
