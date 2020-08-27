#include "eagleeye/engine/nano/op/gpu/opencl_util.h"
#include "eagleeye/basic/Math.h"
#include "eagleeye/common/EagleeyeLog.h"
#include <vector>
#include <utility>


namespace eagleeye{
// [(C + 3) / 4 * W, N * H]
void CalInOutputImageShape(const std::vector<int64_t> &shape, /* NHWC */
                           std::vector<int64_t> *image_shape) {
//   MACE_CHECK(shape.size() == 4);
  image_shape->resize(2);
  (*image_shape)[0] = RoundUpDiv4(shape[3]) * shape[2];
  (*image_shape)[1] = shape[0] * shape[1];
}

// [Ic, H * W * (Oc + 3) / 4]
void CalConv2dFilterImageShape(const std::vector<int64_t> &shape, /* OIHW */
                               std::vector<int64_t> *image_shape) {
//   MACE_CHECK(shape.size() == 4);
  image_shape->resize(2);
  (*image_shape)[0] = shape[1];
  (*image_shape)[1] = shape[2] * shape[3] * RoundUpDiv4(shape[0]);
}

// [H * W * M, (Ic + 3) / 4]
void CalDepthwiseConv2dFilterImageShape(
    const std::vector<int64_t> &shape, /* MIHW */
    std::vector<int64_t> *image_shape) {
//   MACE_CHECK(shape.size() == 4);
  image_shape->resize(2);
  (*image_shape)[0] = shape[0] * shape[2] * shape[3];
  (*image_shape)[1] = RoundUpDiv4(shape[1]);
}

// [W * C, N * RoundUp<4>(H)]
void CalInOutHeightImageShape(const std::vector<int64_t> &shape, /* NHWC */
                              std::vector<int64_t> *image_shape) {
//   MACE_CHECK(shape.size() == 4);
  image_shape->resize(2);
  (*image_shape)[0] = shape[2] * shape[3];
  (*image_shape)[1] = shape[0] * RoundUpDiv4(shape[1]);
}

// [RoundUp<4>(W) * C, N * H]
void CalInOutWidthImageShape(const std::vector<int64_t> &shape, /* NHWC */
                             std::vector<int64_t> *image_shape) {
//   MACE_CHECK(shape.size() == 4);
  image_shape->resize(2);
  (*image_shape)[0] = RoundUpDiv4(shape[2]) * shape[3];
  (*image_shape)[1] = shape[0] * shape[1];
}

// [Ic * H * W, (Oc + 3) / 4]
void CalWeightHeightImageShape(const std::vector<int64_t> &shape, /* OIHW */
                               std::vector<int64_t> *image_shape) {
//   MACE_CHECK(shape.size() == 4);
  image_shape->resize(2);
  (*image_shape)[0] = shape[1] * shape[2] * shape[3];
  (*image_shape)[1] = RoundUpDiv4(shape[0]);
}

// [(Ic + 3) / 4 * H * W, Oc]
void CalWeightWidthImageShape(const std::vector<int64_t> &shape, /* OIHW */
                              std::vector<int64_t> *image_shape) {
//   MACE_CHECK(shape.size() == 4);
  image_shape->resize(2);
  (*image_shape)[0] = RoundUpDiv4(shape[1]) * shape[2] * shape[3];
  (*image_shape)[1] = shape[0];
}


void OpenCLUtil::CalImage2DShape(const std::vector<int64_t> &shape, /* NHWC */
                                 const OpenCLBufferType type,
                                 std::vector<int64_t> *image_shape,
                                 const int wino_block_size) {
  switch (type) {
    case CONV2D_FILTER:
      CalConv2dFilterImageShape(shape, image_shape);
      break;
    case DW_CONV2D_FILTER:
      CalDepthwiseConv2dFilterImageShape(shape, image_shape);
      break;
    case IN_OUT_CHANNEL:
      CalInOutputImageShape(shape, image_shape);
      break;
    case IN_OUT_HEIGHT:
      CalInOutHeightImageShape(shape, image_shape);
      break;
    case IN_OUT_WIDTH:
      CalInOutWidthImageShape(shape, image_shape);
      break;
    case WEIGHT_HEIGHT:
      CalWeightHeightImageShape(shape, image_shape);
      break;
    case WEIGHT_WIDTH:
      CalWeightWidthImageShape(shape, image_shape);
      break;
    default:
      EAGLEEYE_LOGE("dont support yet.");
  }
}
}  // namespace