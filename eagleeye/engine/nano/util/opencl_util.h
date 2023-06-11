#ifndef _EAGLEEYE_OPENCL_UTIL_H_
#define _EAGLEEYE_OPENCL_UTIL_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include <vector>
#include <cstdint>

namespace eagleeye{

enum OpenCLBufferType {
  CONV2D_FILTER = 0,
  IN_OUT_CHANNEL = 1,
  ARGUMENT = 2,
  IN_OUT_HEIGHT = 3,
  IN_OUT_WIDTH = 4,
  WINOGRAD_FILTER = 5,
  DW_CONV2D_FILTER = 6,
  WEIGHT_HEIGHT = 7,
  WEIGHT_WIDTH = 8,
};

class OpenCLUtil {
 public:
  static void CalImage2DShape(const std::vector<int64_t> &shape, /* NHWC */
                              const OpenCLBufferType type,
                              std::vector<int64_t> *image_shape,
                              const int wino_blk_size = 2);
};

}

#endif