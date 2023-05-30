#include <arm_neon.h>

namespace eagleeye {
namespace math {
namespace arm {
void bgrToTensorCHW(const uint8_t* src,
                       float* output,
                       int width,
                       int height,
                       float* means,
                       float* scales);

void bgrToRgbTensorCHW(const uint8_t* src,
                       float* output,
                       int width,
                       int height,
                       float* means,
                       float* scales);
}  // namespace arm
}  // namespace math
}  // namespace eagleeye