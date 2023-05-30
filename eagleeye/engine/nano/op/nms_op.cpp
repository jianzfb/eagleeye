#include "eagleeye/engine/nano/op/nms_op.h"
#include <math.h>

namespace eagleeye{
namespace dataflow{
NmsOp::NmsOp(float score_threshold, 
             int nms_top_k, 
             int keep_top_k, 
             float nms_threshold, 
             float normalized, 
             float nms_eta, 
             int background_label, bool use_gaussian, float gaussian_sigma)
    :m_score_threshold(score_threshold), 
        m_nms_top_k(nms_top_k),
        m_keep_top_k(keep_top_k),
        m_nms_threshold(nms_threshold),
        m_normalized(normalized),
        m_nms_eta(nms_eta),
        m_background_label(background_label),
        m_use_gaussian(use_gaussian),
        m_gaussian_sigma(gaussian_sigma){
    OP_SUPPORT(CPU);
}

NmsOp::NmsOp(const NmsOp& op)
    :m_score_threshold(op.m_score_threshold), 
        m_nms_top_k(op.m_nms_top_k),
        m_keep_top_k(op.m_keep_top_k),
        m_nms_threshold(op.m_nms_threshold),
        m_normalized(op.m_normalized),
        m_nms_eta(op.m_nms_eta),
        m_background_label(op.m_background_label){
    OP_SUPPORT(CPU);
}

NmsOp::~NmsOp(){

}

int NmsOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}


template <class T>
static T BBoxArea(const T* box, const bool normalized) {
  if (box[2] < box[0] || box[3] < box[1]) {
    // If coordinate values are is invalid
    // (e.g. xmax < xmin or ymax < ymin), return 0.
    return static_cast<T>(0.);
  } else {
    const T w = box[2] - box[0];
    const T h = box[3] - box[1];
    if (normalized) {
      return w * h;
    } else {
      // If coordinate values are not within range [0, 1].
      return (w + 1) * (h + 1);
    }
  }
}

template <class T>
static T JaccardOverlap(const T* box1, const T* box2, const bool normalized) {
  if (box2[0] > box1[2] || box2[2] < box1[0] || box2[1] > box1[3] ||
      box2[3] < box1[1]) {
    return static_cast<T>(0.);
  } else {
    const T inter_xmin = (std::max)(box1[0], box2[0]);
    const T inter_ymin = (std::max)(box1[1], box2[1]);
    const T inter_xmax = (std::min)(box1[2], box2[2]);
    const T inter_ymax = (std::min)(box1[3], box2[3]);
    T norm = normalized ? static_cast<T>(0.) : static_cast<T>(1.);
    T inter_w = inter_xmax - inter_xmin + norm;
    T inter_h = inter_ymax - inter_ymin + norm;
    const T inter_area = inter_w * inter_h;
    const T bbox1_area = BBoxArea<T>(box1, normalized);
    const T bbox2_area = BBoxArea<T>(box2, normalized);
    return inter_area / (bbox1_area + bbox2_area - inter_area);
  }
}

template <class T>
T PolyIoU(const T* box1,
          const T* box2,
          const size_t box_size,
          const bool normalized) {
  return *box1;
}

template <typename T, bool gaussian>
struct decay_score;

template <typename T>
struct decay_score<T, true> {
  T operator()(T iou, T max_iou, T sigma) {
    return exp((max_iou * max_iou - iou * iou) * sigma);
  }
};

template <typename T>
struct decay_score<T, false> {
  T operator()(T iou, T max_iou, T sigma) {
    return (1. - iou) / (1. - max_iou);
  }
};

template <typename T, bool gaussian>
void NMSMatrix(const Tensor& bbox,
               const Tensor& scores,
               const T score_threshold,
               const T post_threshold,
               const float sigma,
               const int64_t top_k,
               const bool normalized,
               std::vector<int>* selected_indices,
               std::vector<T>* decayed_scores) {
    int64_t num_boxes = bbox.dims()[0];
    int64_t box_size = bbox.dims()[1];

    //   auto score_ptr = scores.data<T>();
    //   auto bbox_ptr = bbox.data<T>();
    const T* score_ptr = scores.cpu<T>();
    const T* bbox_ptr = bbox.cpu<T>();

    std::vector<int32_t> perm(num_boxes);
    std::iota(perm.begin(), perm.end(), 0);
    auto end = std::remove_if(
        perm.begin(), perm.end(), [&score_ptr, score_threshold](int32_t idx) {
            return score_ptr[idx] <= score_threshold;
        });
    auto sort_fn = [&score_ptr](int32_t lhs, int32_t rhs) {
        return score_ptr[lhs] > score_ptr[rhs];
    };

    int64_t num_pre = std::distance(perm.begin(), end);
    if (num_pre <= 0) {
        return;
    }
    if (top_k > -1 && num_pre > top_k) {
        num_pre = top_k;
    }
    std::partial_sort(perm.begin(), perm.begin() + num_pre, end, sort_fn);

    std::vector<T> iou_matrix((num_pre * (num_pre - 1)) >> 1);
    std::vector<T> iou_max(num_pre);

    iou_max[0] = 0.;
    for (int64_t i = 1; i < num_pre; i++) {
        T max_iou = 0.;
        auto idx_a = perm[i];
        for (int64_t j = 0; j < i; j++) {
            auto idx_b = perm[j];
            auto iou = JaccardOverlap<T>(
                bbox_ptr + idx_a * box_size, bbox_ptr + idx_b * box_size, normalized);
            max_iou = (std::max)(max_iou, iou);
            iou_matrix[i * (i - 1) / 2 + j] = iou;
        }
        iou_max[i] = max_iou;
    }

    if (score_ptr[perm[0]] > post_threshold) {
        selected_indices->push_back(perm[0]);
        decayed_scores->push_back(score_ptr[perm[0]]);
    }

    decay_score<T, gaussian> decay_fn;
    for (int64_t i = 1; i < num_pre; i++) {
        T min_decay = 1.;
        for (int64_t j = 0; j < i; j++) {
            auto max_iou = iou_max[j];
            auto iou = iou_matrix[i * (i - 1) / 2 + j];
            auto decay = decay_fn(iou, max_iou, sigma);
            min_decay = (std::min)(min_decay, decay);
        }
        auto ds = min_decay * score_ptr[perm[i]];
        if (ds <= post_threshold) continue;
        selected_indices->push_back(perm[i]);
        decayed_scores->push_back(ds);
    }
}

template <typename T>
size_t MultiClassMatrixNMS(const Tensor& scores,
                           const Tensor& bboxes,
                           std::vector<T>* out,
                           std::vector<int>* indices,
                           int start,
                           int64_t background_label,
                           int64_t nms_top_k,
                           int64_t keep_top_k,
                           bool normalized,
                           T score_threshold,
                           T post_threshold,
                           bool use_gaussian,
                           float gaussian_sigma) {
    std::vector<int> all_indices;
    std::vector<T> all_scores;
    std::vector<T> all_classes;
    all_indices.reserve(scores.numel());
    all_scores.reserve(scores.numel());
    all_classes.reserve(scores.numel());

    size_t num_det = 0;
    auto class_num = scores.dims()[0];
    Tensor score_slice;
    for (int64_t c = 0; c < class_num; ++c) {
        if (c == background_label) continue;
        score_slice = scores.slice(c, c + 1);
        if (use_gaussian) {
            NMSMatrix<T, true>(bboxes,
                                score_slice,
                                score_threshold,
                                post_threshold,
                                gaussian_sigma,
                                nms_top_k,
                                normalized,
                                &all_indices,
                                &all_scores);
        } else {
            NMSMatrix<T, false>(bboxes,
                                score_slice,
                                score_threshold,
                                post_threshold,
                                gaussian_sigma,
                                nms_top_k,
                                normalized,
                                &all_indices,
                                &all_scores);
        }

        for (size_t i = 0; i < all_indices.size() - num_det; i++) {
            all_classes.push_back(static_cast<T>(c));
        }
        num_det = all_indices.size();
    }

    if (num_det <= 0) {
        return num_det;
    }

    if (keep_top_k > -1) {
        auto k = static_cast<size_t>(keep_top_k);
        if (num_det > k) num_det = k;
    }

    std::vector<int32_t> perm(all_indices.size());
    std::iota(perm.begin(), perm.end(), 0);

    std::partial_sort(perm.begin(),
                        perm.begin() + num_det,
                        perm.end(),
                        [&all_scores](int lhs, int rhs) {
                            return all_scores[lhs] > all_scores[rhs];
                        });

    for (size_t i = 0; i < num_det; i++) {
        auto p = perm[i];
        auto idx = all_indices[p];
        auto cls = all_classes[p];
        auto score = all_scores[p];
        auto bbox = bboxes.cpu<T>() + idx * bboxes.dims()[1];
        (*indices).push_back(start + idx);
        (*out).push_back(cls);
        (*out).push_back(score);
        for (int j = 0; j < bboxes.dims()[1]; j++) {
            (*out).push_back(bbox[j]);
        }
    }

    return num_det;
}


int NmsOp::runOnCpu(const std::vector<Tensor>& input){
    const Tensor boxes = input[0];        // N,M,4
    const Tensor scores = input[1];       // N,C,M
    
    Dim score_dims = scores.dims(); // N,C,M
    int batch_size = score_dims[0];
    int num_boxes = score_dims[2];
    int box_dim = boxes.dims()[2];  // N,M,4
    int out_dim = box_dim + 2;

    Tensor boxes_slice, scores_slice;
    int64_t num_out = 0;
    std::vector<int64_t> offsets = {0};
    std::vector<float> detections;
    std::vector<int> indices;
    std::vector<int> num_per_batch;
    detections.reserve(out_dim * num_boxes * batch_size);
    indices.reserve(num_boxes * batch_size);
    num_per_batch.reserve(batch_size);
    for (int i = 0; i < batch_size; ++i) {
        scores_slice = scores.slice(i, i+1);
        scores_slice = scores_slice.squeeze(0);

        boxes_slice = boxes.slice(i, i+1);
        boxes_slice = boxes_slice.squeeze(0);

        int start = i * score_dims[2];
        num_out = MultiClassMatrixNMS(scores_slice,
                                    boxes_slice,
                                    &detections,
                                    &indices,
                                    start,
                                    m_background_label,
                                    m_nms_top_k,
                                    m_keep_top_k,
                                    m_normalized,
                                    m_score_threshold,
                                    m_nms_threshold,
                                    m_use_gaussian,
                                    m_gaussian_sigma);
        offsets.push_back(offsets.back() + num_out);
        num_per_batch.emplace_back(num_out);
    }
    if(detections.size() == 0){
        return 0;
    }

    // detections
    // label, confidence, xmin, ymin, xmax, ymax, label, confidence, xmin, ymin, xmax, ymax
    Dim output_dim = this->m_outputs[0].dims();
    if(output_dim.empty() || output_dim.production() != detections.size()){
        int64_t num = detections.size()/6;
        this->m_outputs[0] = \
            Tensor(std::vector<int64_t>{num, 6}, EAGLEEYE_FLOAT, DataFormat::AUTO, CPU_BUFFER);
    }
    memcpy(this->m_outputs[0].cpu(), detections.data(), sizeof(float)*detections.size());
    return 0;
}

int NmsOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}

} // namespace dataflow
} // namespace eagleeye
